#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__

#include <memory>

namespace sylar
{

    class Stream
    {
    public:
        using ptr = std::shared_ptr<Stream>;

        virtual ~Stream() {}

        // 读数据
        virtual int read(void* buffer, size_t length) = 0;

        // 读固定长度的数据
        virtual int readFixSize(void* buffer, size_t length);

        // 写数据
        virtual int write(const void* buffer, size_t length) = 0;

        // 写固定长度的数据
        virtual int writeFixSize(const void* buffer, size_t length);

        // 关闭流
        virtual void close() = 0;
    };

}

#endif