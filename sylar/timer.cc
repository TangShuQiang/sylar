#include "timer.h"
#include "util.h"

namespace sylar
{
    bool Timer::Comparator::operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const {
        if (!lhs && !rhs) {
            return false;
        }
        if (!lhs) {
            return true;
        }
        if (!rhs) {
            return false;
        }
        if (lhs->m_next != rhs->m_next) {
            return lhs->m_next < rhs->m_next;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t next) : m_next(next) {}

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
        m_next = sylar::GetCurrentMS() + m_ms;
    }

    bool Timer::cancel() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            m_cb = nullptr;
            m_manager->m_timers.erase(shared_from_this());
            return true;
        }
        return false;
    }

    bool Timer::refresh() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = sylar::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now) {
        if (ms == m_ms && !from_now) {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        if (from_now) {
            m_next = sylar::GetCurrentMS() + ms;
        } else {
            m_next = m_next - m_ms + ms;
        }
        m_ms = ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    // ------------------------------------------------------------------------
    TimerManager::TimerManager() {
        m_previousTime = sylar::GetCurrentMS();
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }

    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
        auto it = m_timers.insert(val).first;
        if (it == m_timers.begin() && m_tickled == false) {
            m_tickled = true;
            lock.unlock();
            onTimerInsertedAtFront();
        }
    }

    static void Ontimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp) {
            cb();
        }
    }

    // 添加条件定时器
    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring) {
        return addTimer(ms, std::bind(&Ontimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty()) {
            return ~0ull;
        }
        Timer::ptr first = *m_timers.begin();
        uint64_t cur_ms = sylar::GetCurrentMS();
        if (first->m_next > cur_ms) {
            return first->m_next - cur_ms;
        }
        return 0;
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>>& vec) {
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty()) {
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
        uint64_t cur_ms = sylar::GetCurrentMS();
        m_previousTime = cur_ms;
        std::vector<Timer::ptr> expired;
        Timer::ptr curTimer(new Timer(cur_ms));
        auto it = m_timers.upper_bound(curTimer);
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        vec.reserve(expired.size());
        for (Timer::ptr& timer : expired) {
            vec.push_back(timer->m_cb);
            if (timer->m_recurring) {
                timer->m_next = cur_ms + timer->m_ms;
                m_timers.insert(timer);
            }
        }
    }

}