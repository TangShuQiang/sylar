#ifndef __SYLAR_FD_MANAGER_H__
#define __SYLAR_FD_MANAGER_H__

#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"

namespace sylar
{
    class FdCtx
    {
    public:
        using ptr = std::shared_ptr<FdCtx>;
        FdCtx(int fd);

        bool isSocket() const { return m_isSocket; }
        bool getSysNonblock() const { return m_sysNonblock; }
        bool isUserNonblock() const { return m_userNonblock; }
        bool isClosed() const { return m_isClosed; }

        void setSysNonblock(bool v) { m_sysNonblock = v; }
        void setUserNonblock(bool v) { m_userNonblock = v; }

        uint64_t getTimeout(int type);
        void setTimeout(int type, uint64_t v);

    private:
        bool init();

    private:
        bool m_isSocket;
        bool m_sysNonblock;             // 是否hook非阻塞
        bool m_userNonblock;            // 是否用户主动设置非阻塞
        bool m_isClosed;
        int m_fd;
        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
    };

    class FdManager
    {
    public:
        using RWMutexType = RWMutex;
        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);
    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::ptr> m_datas;
    };

}


#endif