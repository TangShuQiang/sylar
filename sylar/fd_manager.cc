#include "fd_manager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

namespace sylar
{
    FdCtx::FdCtx(int fd) : m_isSocket(false), m_sysNonblock(false), m_userNonblock(false),
        m_isClosed(false), m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1) {
        init();
    }

    bool FdCtx::init() {
        struct stat fd_stat;
        if (-1 == fstat(m_fd, &fd_stat)) {
            return false;
        }
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
        if (m_isSocket) {
            int flag = fcntl_f(m_fd, F_GETFL, 0);
            if (!(flag & O_NONBLOCK)) {
                fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }
        return true;
    }

    uint64_t FdCtx::getTimeout(int type) {
        if (type == SO_RCVTIMEO) {
            return m_recvTimeout;
        }
        return m_sendTimeout;
    }

    void FdCtx::setTimeout(int type, uint64_t v) {
        if (type == SO_RCVTIMEO) {
            m_recvTimeout = v;
        } else {
            m_sendTimeout = v;
        }
    }

    // ----------------------------------------------------------
    FdManager::FdManager() : m_datas(64) {}

    FdCtx::ptr FdManager::get(int fd, bool auto_create) {
        if (fd < 0) {
            return nullptr;
        }
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (fd >= (int)m_datas.size()) {
                if (!auto_create) {
                    return nullptr;
                }
            } else {
                if (m_datas[fd] || !auto_create) {
                    return m_datas[fd];
                }
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if (fd >= (int)m_datas.size()) {
            m_datas.resize(fd * 1.5);
        }
        if (!m_datas[fd]) {
            m_datas[fd] = std::make_shared<FdCtx>(fd);
        }
        return m_datas[fd];
    }

    void FdManager::del(int fd) {
        RWMutexType::WriteLock lock(m_mutex);
        if (fd < (int)m_datas.size()) {
            m_datas[fd] = nullptr;
        }
    }

}