#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include <fcntl.h>

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    IOManager* IOManager::GetThisIOManager() {
        return dynamic_cast<IOManager*>(Scheduler::GetThisScheduler());
    }

    IOManager::IOManager(size_t threadCount, bool usecaller, const std::string& name)
        : Scheduler(threadCount, usecaller, name) {
        m_epollfd = epoll_create(1);
        SYLAR_ASSERT(m_epollfd != -1);
        m_eventfd = eventfd(0, EFD_NONBLOCK);
        SYLAR_ASSERT(m_eventfd != -1);
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLET | EPOLLIN;
        event.data.fd = m_eventfd;
        int ret = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_eventfd, &event);
        SYLAR_ASSERT(ret != -1);
        contextResize(32);
        start();
    }

    IOManager::~IOManager() {
        stop();
        close(m_eventfd);
        close(m_epollfd);
        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
        FdContext* fdcontext;
        {
            RWMutexType::ReadLock readlock(m_mutex);
            if (fd < (int)m_fdContexts.size()) {
                fdcontext = m_fdContexts[fd];
            } else {
                readlock.unlock();
                RWMutexType::WriteLock writelock(m_mutex);
                contextResize(fd * 1.5);
                fdcontext = m_fdContexts[fd];
            }
        }
        FdContext::MutexType::Lock lock(fdcontext->mutex);
        if (fdcontext->events & event) {
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                << " event=" << event
                << " fdcontext->events=" << fdcontext->events;
            SYLAR_ASSERT(false);
        }
        int op = fdcontext->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        struct epoll_event epollevent;
        memset(&epollevent, 0, sizeof(epollevent));
        epollevent.events = EPOLLET | fdcontext->events | event;
        epollevent.data.ptr = fdcontext;
        int ret = epoll_ctl(m_epollfd, op, fd, &epollevent);
        if (ret == -1) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                << op << ", " << fd << ", " << epollevent.events << "): " << strerror(errno);
            return -1;
        }
        ++m_pendingEventCount;
        fdcontext->events = (Event)(fdcontext->events | event);
        EventContext& eventContext = fdcontext->getContext(event);
        SYLAR_ASSERT(!eventContext.scheduler && !eventContext.fiber && !eventContext.cb);
        eventContext.scheduler = Scheduler::GetThisScheduler();
        if (cb) {
            eventContext.cb.swap(cb);
        } else {
            eventContext.fiber = Fiber::GetThis()->shared_from_this();
            SYLAR_ASSERT2(eventContext.fiber->getState() == Fiber::State::EXEC,
                "state=" << eventContext.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event) {
        RWMutex::ReadLock readlock(m_mutex);
        if (fd >= (int)m_fdContexts.size()) {
            return false;
        }
        FdContext* fdcontext = m_fdContexts[fd];
        readlock.unlock();
        FdContext::MutexType::Lock lock(fdcontext->mutex);
        if (!(fdcontext->events & event)) {
            return false;
        }
        Event newEvents = (Event)(fdcontext->events & ~event);
        int op = newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        struct epoll_event epollevent;
        memset(&epollevent, 0, sizeof(epollevent));
        epollevent.events = EPOLLET | newEvents;
        epollevent.data.ptr = fdcontext;
        int ret = epoll_ctl(m_epollfd, op, fd, &epollevent);
        if (ret == -1) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                << op << ", " << fd << ", " << epollevent.events << "): " << strerror(errno);
            return false;
        }
        --m_pendingEventCount;
        fdcontext->events = newEvents;
        fdcontext->resetContext(event);
        return true;
    }

    void IOManager::contextResize(size_t size) {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext();
                m_fdContexts[i]->fd = i;
            }
        }
    }

    // ----------------------------------------------------------
    void IOManager::tickle() {
        if (hasIdleThreads()) {
            uint64_t one = 1;
            ssize_t ret = write(m_eventfd, &one, sizeof(uint64_t));
            SYLAR_ASSERT(ret == sizeof(uint64_t));
        }
    }

    bool IOManager::stopping() {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    bool IOManager::stopping(uint64_t& timeout) {
        timeout = getNextTimer();
        return timeout == ~0ull
            && Scheduler::stopping()
            && m_pendingEventCount == 0;
    }

    void IOManager::onTimerInsertedAtFront() {
        tickle();
    }

    void IOManager::idle() {
        SYLAR_LOG_DEBUG(g_logger) << "idle";
        const uint64_t MAX_EVENTS = 256;
        struct epoll_event* readyEvents = new struct epoll_event[MAX_EVENTS]();
        std::shared_ptr<struct epoll_event[]> shared_events(readyEvents);
        while (true) {
            uint64_t next_timeout = 0;
            if (stopping(next_timeout)) {
                SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
                break;
            }
            int readyNum = 0;
            while (true) {
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull) {
                    next_timeout = ((int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout);
                } else {
                    next_timeout = MAX_TIMEOUT;
                }
                readyNum = epoll_wait(m_epollfd, readyEvents, MAX_EVENTS, (int)next_timeout);
                if (readyNum > 0) {
                    SYLAR_LOG_DEBUG(g_logger) << "epoll_wait ready, ret = " << readyNum;
                    break;
                } else if (readyNum == 0) {
                    SYLAR_LOG_DEBUG(g_logger) << "epoll_wait timeout";
                    break;
                } else {
                    if (errno == EINTR) {
                        continue;
                    } else {
                        SYLAR_LOG_ERROR(g_logger) << "epoll_wait: " << strerror(errno);
                    }
                }
            }
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty()) {
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }
            for (int i = 0; i < readyNum; ++i) {
                struct epoll_event& event = readyEvents[i];
                if (event.data.fd == m_eventfd) {
                    uint64_t two;
                    ssize_t ret = read(m_eventfd, &two, sizeof(uint64_t));
                    SYLAR_ASSERT(ret == sizeof(uint64_t));
                    continue;
                }
                FdContext* fdcontext = (FdContext*)event.data.ptr;
                FdContext::MutexType::Lock lock(fdcontext->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP)) {
                    event.events |= (EPOLLIN | EPOLLOUT) & fdcontext->events;
                }
                Event real_events = Event::NONE;
                if (event.events & EPOLLIN) {
                    real_events = (Event)(real_events | Event::READ);
                }
                if (event.events & EPOLLOUT) {
                    real_events = (Event)(real_events | Event::WRITE);
                }
                if ((fdcontext->events & real_events) == Event::NONE) {
                    continue;
                }
                Event left_events = (Event)(fdcontext->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int ret = epoll_ctl(m_epollfd, op, fdcontext->fd, &event);
                if (ret == -1) {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epollfd << ", "
                        << op << ", " << fdcontext->fd << ", " << event.events << "): " << strerror(errno);
                    continue;
                }
                if (real_events & Event::READ) {
                    fdcontext->triggerEvent(Event::READ);
                    --m_pendingEventCount;
                }
                if (real_events & Event::WRITE) {
                    fdcontext->triggerEvent(Event::WRITE);
                    --m_pendingEventCount;
                }
            }
            sylar::Fiber::YieldToHold();
        }
    }


    // ----------------------------------------------------------
    // 获取事件上下文类
    IOManager::EventContext& IOManager::FdContext::getContext(Event event) {
        switch (event) {
            case Event::READ:
                return readEventContext;
            case Event::WRITE:
                return writeEventContext;
            default:
                SYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid enent");
    }

    // 重置事件上下文
    void IOManager::FdContext::resetContext(Event event) {
        EventContext& eventcontext = getContext(event);
        eventcontext.scheduler = nullptr;
        eventcontext.fiber = nullptr;
        eventcontext.cb = nullptr;
    }

    // 触发事件
    void IOManager::FdContext::triggerEvent(Event event) {
        SYLAR_ASSERT(event & events);
        events = (Event)(events & ~event);
        EventContext eventContext = getContext(event);
        if (eventContext.cb) {
            eventContext.scheduler->schedule(eventContext.cb);
            eventContext.cb = nullptr;
        } else {
            eventContext.scheduler->schedule(eventContext.fiber);
            eventContext.fiber = nullptr;
        }
        eventContext.scheduler = nullptr;
    }

}