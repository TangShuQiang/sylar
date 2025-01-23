#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <stdint.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

namespace sylar
{
    class ByteArray
    {
    public:
        using ptr = std::shared_ptr<ByteArray>;

        // 内存块
        struct Node
        {
            Node();
            Node(size_t s);
            ~Node();

            char* ptr;
            Node* next;
            size_t size;
        };

        ByteArray(size_t baseSize = 4096);
        ~ByteArray();

        // 写入固定长度int8_t类型的数据
        void writeFint8(int8_t value);
        void writeFuint8(uint8_t value);
        void writeFint16(int16_t value);
        void writeFuint16(uint16_t value);
        void writeFint32(int32_t value);
        void writeFuint32(uint32_t value);
        void writeFint64(int64_t value);
        void writeFuint64(uint64_t value);

        // 变长写入int32_t类型的数据
        void writeInt32(int32_t value);
        void writeUint32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUint64(uint64_t value);
        void writeFloat(float value);
        void writeDouble(double value);

        // 写入std::string类型的数据，用uint_16作为长度类型
        void writeStringF16(const std::string& value);
        void writeStringF32(const std::string& value);
        void writeStringF64(const std::string& value);

        // 写入std::string类型的数据，用无符号varint64作为长度类型
        void writeStringVint(const std::string& value);

        void writeStringWithoutLength(const std::string& value);

        int8_t readFint8();
        uint8_t readFuint8();
        int16_t readFint16();
        uint16_t readFuint16();
        int32_t readFint32();
        uint32_t readFuint32();
        int64_t readFint64();
        uint64_t readFuint64();

        int32_t readInt32();
        uint32_t readUint32();
        int64_t readInt64();
        uint64_t readUint64();
        float readFloat();
        double readDouble();

        std::string readStringF16();
        std::string readStringF32();
        std::string readStringF64();

        std::string StringVint();

        // 清空ByteArray
        void clear();

        // 写入size长度的数据
        void write(const void* buf, size_t size);

        // 读取size长度的数据
        void read(void* buf, size_t size);

        // 从指定位置position读取size长度的数据
        void read(void* buf, size_t size, size_t position) const;

        // 把ByteArray的数据写入文件中
        bool writeToFile(const std::string& name) const;

        // 从文件中读取数据
        bool readFromFile(const std::string& name);

        // 将ByteArray里面的数据[m_position, m_size]转成std::string
        std::string toString() const;

        // 将ByteArray里面的数据[m_position, m_size]转成16进制的std::string
        std::string toHexString() const;

        /*  
            获取可写入的缓存，保存成iovec数组
            len: 写入的长度，如果m_position + len > m_capacity，则m_capacity扩容
            返回实际的长度
        */
        // uint64_t getWriteBuffer(std::vector<iovec>& buffers, uint64_t len);

        /*
            读取可读取的缓存，保存成iovec数组
            len: 读取数据的长度，如果len > getReadSize()，则len = getReadSize();
            返回实际数据的长度
        */ 
        // uint64_t getReadBuffer(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;

        /*
            从position位置开始，读取可读取的缓存，保存成iovec数组
            len: 读取数据的长度，如果len > getReadSize()，则len = getReadSize();
            返回实际数据的长度
        */ 
        // uint64_t getReadBuffer(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;

        // 返回内存块的大小
        size_t getBaseSize() const { return m_baseSize; }

        // 返回可读数据的大小
        size_t getReadSize() const { return m_size - m_position; }

        // 返回数据的长度
        size_t getSize() const { return m_size; }

        // 返回BtyeArray当前位置
        size_t getPosition() const { return m_position; }

        // 设置ByteArray当前位置
        void setPosition(size_t v);

    private:
        // 获取当前的可写入容量
        size_t getCapacity() const { return m_capacity - m_position; }

        // 扩容ByteArray，使其可以容纳size个数据，如果原本可以容纳，则不扩容
        void addCapacity(size_t size);

    private:
        size_t m_baseSize;      // 内存块大小
        size_t m_position;      // 当前操作位置
        size_t m_capacity;      // 当前的容量
        size_t m_size;          // 当前数据的大小
        Node* m_root;           // 第一个内存块的指针
        Node* m_cur;            // 当前操作的内存块指针

    };
}

#endif