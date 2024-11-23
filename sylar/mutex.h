#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>
#include "noncopyable.h"
#include <stdexcept>

namespace sylar
{
    class Semaphore : Noncopyable
    {
    public:
        Semaphore(uint32_t count = 0) {
            if (sem_init(&m_semaphore, 0, count)) {
                throw std::logic_error("sem_init error");
            }
        }
        ~Semaphore() {
            sem_destroy(&m_semaphore);
        }
        void wait() {
            if (sem_wait(&m_semaphore)) {
                throw std::logic_error("sem_wait error");
            }
        }
        void notify() {
            if (sem_post(&m_semaphore)) {
                throw std::logic_error("sem_post error");
            }
        }
    private:
        sem_t m_semaphore;
    };

    template<typename T>
    class ScopedLockImpl
    {
    public:
        ScopedLockImpl(T& mutex) : m_mutex(mutex), m_locked(false) { lock(); }
        ~ScopedLockImpl() { unlock(); }
        void lock() {
            if (!m_locked) {
                m_mutex.lock();
                m_locked = true;
            }
        }
        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    template<typename T>
    class ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T& mutex) : m_mutex(mutex), m_locked(false) { lock(); }
        ~ReadScopedLockImpl() { unlock(); }
        void lock() {
            if (!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    template<typename T>
    class WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T& mutex) : m_mutex(mutex), m_locked(false) { lock(); }
        ~WriteScopedLockImpl() { unlock(); }
        void lock() {
            if (!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }
        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    class Mutex : Noncopyable
    {
    public:
        using Lock = ScopedLockImpl<Mutex>;
        Mutex() {
            pthread_mutex_init(&m_mutex, nullptr);
        }
        ~Mutex() {
            pthread_mutex_destroy(&m_mutex);
        }
        void lock() {
            pthread_mutex_lock(&m_mutex);
        }
        void unlock() {
            pthread_mutex_unlock(&m_mutex);
        }
    private:
        pthread_mutex_t m_mutex;
    };

    class RWMutex : Noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;
        RWMutex() {
            pthread_rwlock_init(&m_lock, nullptr);
        }
        ~RWMutex() {
            pthread_rwlock_destroy(&m_lock);
        }
        void rdlock() {
            pthread_rwlock_rdlock(&m_lock);
        }
        void wrlock() {
            pthread_rwlock_wrlock(&m_lock);
        }
        void unlock() {
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        pthread_rwlock_t m_lock;
    };

}

#endif