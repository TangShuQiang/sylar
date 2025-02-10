#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <vector>

#include "noncopyable.h"
#include "socket.h"
#include "iomanager.h"

namespace sylar
{
    class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;

        TcpServer(IOManager* worker = IOManager::GetThisIOManager()
            , IOManager* ioWorker = IOManager::GetThisIOManager()
            , IOManager* acceptWorker = IOManager::GetThisIOManager());

        virtual ~TcpServer();

        // 绑定地址
        virtual bool bind(Address::ptr addr, bool ssl = false);
        virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl = false);

        // 启动服务
        virtual bool start();

        // 停止服务
        virtual void stop();

        uint64_t getRecvTimeout() const { return m_recvTimeout; }
        std::string getName()const { return m_name; }
        bool isStop() const { return m_isStop; }
        std::vector<Socket::ptr> getSocks() const { return m_socks; }

        void setRecvTimeout(uint64_t t) { m_recvTimeout = t; }
        virtual void setName(const std::string& name) { m_name = name; }

        virtual std::string toString(const std::string& prefix = "");

    protected:
        // 处理新连接的Socket类
        virtual void handleClient(Socket::ptr client);

        // 开始接受连接
        virtual void startAccept(Socket::ptr sock);

    private:
        std::vector<Socket::ptr> m_socks;   // 监听Socket数组
        IOManager* m_worker;                // 新连接的Socket工作的调度器
        IOManager* m_ioWorker;              // 新连接的Socket工作的调度器
        IOManager* m_acceptWorker;          // 服务器Socket接收连接的调度器
        uint64_t m_recvTimeout;             // 接收超时时间（毫秒）
        std::string m_name;                 // 服务器名称
        std::string m_type = "tcp";         // 服务器类型
        bool m_isStop;                      // 服务是否停止
        bool m_ssl = false;
    };
}

#endif