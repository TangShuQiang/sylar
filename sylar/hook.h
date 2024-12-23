#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <unistd.h>

namespace sylar
{
    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C" {
    using sleep_fun = unsigned int(*)(unsigned int);
    extern sleep_fun sleep_f;

    using usleep_fun = int(*)(useconds_t);
    extern usleep_fun usleep_f;

    // socket
    using socket_fun = int(*)(int, int, int);
    extern socket_fun socket_f;

    using accept_fun = int(*)(int, struct sockaddr*, socklen_t*);
    extern accept_fun accept_f;

    using connect_fun = int(*)(int, const struct sockaddr*, socklen_t);
    extern connect_fun connect_f;

    // read
    using read_fun = ssize_t(*)(int, void*, size_t);
    extern read_fun read_f;

    using recv_fun = ssize_t(*)(int, void*, size_t, int);
    extern recv_fun recv_f;

    using recvfrom_fun = ssize_t(*)(int, void*, size_t, int, struct sockaddr*, socklen_t*);
    extern recvfrom_fun recvfrom_f;

    // write
    using write_fun = ssize_t(*)(int, const void*, size_t);
    extern write_fun write_f;

    using send_fun = ssize_t(*)(int, const void*, size_t, int);
    extern send_fun send_f;

    using sendto_fun = ssize_t(*)(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
    extern sendto_fun sendto_f;

    // close
    using close_fun = int(*)(int);
    extern close_fun close_f;

    // -------------------------------
    using fcntl_fun = int(*)(int, int, ...);
    extern fcntl_fun fcntl_f;

    using getsockopt_fun = int(*)(int, int, int, void*, socklen_t*);
    extern getsockopt_fun getsockopt_f;

    using setsockopt_fun = int(*)(int, int, int, const void*, socklen_t);
    extern setsockopt_fun setsockopt_f;

}

#endif