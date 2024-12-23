#include "log.h"
#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include <dlfcn.h>
#include <functional>
#include <sys/socket.h>
#include <fcntl.h>

#define HOOK_FUN(XX)    \
    XX(sleep)           \
    XX(usleep)          \
    XX(socket)          \
    XX(accept)          \
    XX(connect)         \
    XX(read)            \
    XX(recv)            \
    XX(recvfrom)        \
    XX(write)           \
    XX(send)            \
    XX(sendto)          \
    XX(close)           \
    XX(fcntl)           \
    XX(getsockopt)      \
    XX(setsockopt)


static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar
{
    static thread_local bool t_hook_enable = false;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

    struct _HookIniter
    {
        _HookIniter() {
            hook_init();
        }

        void hook_init() {
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
            HOOK_FUN(XX);
        #undef XX
        }
    };
    static _HookIniter s_hook_initer;
}


struct timer_info
{
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, uint32_t event, int timeoutType, Args&&... args) {
    if (!sylar::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }
    if (ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }
    uint64_t timeout = ctx->getTimeout(timeoutType);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    while (true) {
        ssize_t n;
        do {
            n = fun(fd, std::forward<Args>(args)...);
        } while (n == -1 && errno == EINTR);
        if (n == -1 && errno == EAGAIN) {
            sylar::IOManager* iom = sylar::IOManager::GetThisIOManager();
            sylar::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);
            if (timeout != (uint64_t)-1) {
                timer = iom->addConditionTimer(timeout, [&]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
                    }, winfo);
            }

            int ret = iom->addEvent(fd, (sylar::IOManager::Event)(event));
            if (ret == -1) {
                SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
                if (timer) {
                    timer->cancel();
                }
                return -1;
            }
            sylar::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            continue;
        }
        return n;
    }
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(unsigned int seconds) {
        if (!sylar::t_hook_enable) {
            return sleep_f(seconds);
        }
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis()->shared_from_this();
        sylar::IOManager* iom = sylar::IOManager::GetThisIOManager();
        iom->addTimer(seconds * 1000,
            std::bind(
                (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int)) & sylar::IOManager::schedule,
                iom, fiber, -1),
            false);
        sylar::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec) {
        if (!sylar::t_hook_enable) {
            return usleep_f(usec);
        }
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis()->shared_from_this();
        sylar::IOManager* iom = sylar::IOManager::GetThisIOManager();
        iom->addTimer(usec / 1000,
            std::bind(
                (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int)) & sylar::IOManager::schedule,
                iom, fiber, -1),
            false);
        sylar::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol) {
        if (!sylar::t_hook_enable) {
            return socket_f(domain, type, protocol);
        }
        int sockfd = socket_f(domain, type, protocol);
        sylar::FdMgr::GetInstance()->get(sockfd, true);
        return sockfd;
    }

    int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
        int fd = do_io(sockfd, accept_f, "accept", sylar::IOManager::Event::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0) {
            sylar::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    // int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {

    // }

    // int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {

    // }

    ssize_t read(int fd, void* buf, size_t count) {
        return do_io(fd, read_f, "read", sylar::IOManager::Event::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
        return do_io(sockfd, recv_f, "recv", sylar::IOManager::Event::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
        return do_io(sockfd, recvfrom_f, "recv", sylar::IOManager::Event::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t write(int fd, const void* buf, size_t count) {
        return do_io(fd, write_f, "write", sylar::IOManager::Event::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
        return do_io(sockfd, send_f, "send", sylar::IOManager::Event::WRITE, SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen) {
        return do_io(sockfd, sendto_f, "sendto", sylar::IOManager::Event::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    int close(int fd) {
        if (!sylar::t_hook_enable) {
            return close_f(fd);
        }
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        if (ctx) {
            sylar::IOManager* iom = sylar::IOManager::GetThisIOManager();
            if (iom) {
                iom->cancelAll(fd);
            }
            sylar::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ...) {
        va_list va;
        va_start(va, cmd);
        switch (cmd) {
            case F_GETFL: {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return arg;
                }
                if (ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            case F_SETFL: {
                int arg = va_arg(va, int);
                va_end(va);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            #ifdef F_SETPIPE_SZ
            case F_SETPIPE_SZ:
            #endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }

            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            #ifdef F_GETPIPE_SZ
            case F_GETPIPE_SZ:
            #endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }

            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }

            case F_GETOWN_EX:
            case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }

            default:
                va_end(va);
                return fcntl_f(fd, cmd);
        }
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
        if (!sylar::t_hook_enable) {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET) {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
                if (ctx) {
                    const timeval* tv = (const timeval*)optval;
                    ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }


}