#include <string.h>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <iostream>

#include "bytearray.h"
#include "log.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    ByteArray::Node::Node()
        : ptr(nullptr)
        , next(nullptr)
        , size(0) {}

    ByteArray::Node::Node(size_t s)
        : ptr(new char[s])
        , next(nullptr)
        , size(s) {}

    ByteArray::Node::~Node() {
        if (ptr) {
            delete[] ptr;
            ptr = nullptr;
        }
    }

    ByteArray::ByteArray(size_t baseSize)
        : m_baseSize(baseSize)
        , m_position(0)
        , m_capacity(baseSize)
        , m_size(0)
        , m_root(new Node(baseSize))
        , m_cur(m_root) {}

    ByteArray::~ByteArray() {
        Node* tmp = m_root;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }

    void ByteArray::writeFint8(int8_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint8(uint8_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint16(int16_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint16(uint16_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint32(int32_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint32(uint32_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFint64(int64_t value) {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint64(uint64_t value) {
        write(&value, sizeof(value));
    }

    // 将 -3、-2、-1、0、1、2、3 转变成 5 3 1 0 1 4 6
    static uint32_t EncodeZigzag32(int32_t v) {
        return (v << 1) ^ (v >> 31);
    }

    static uint64_t EncodeZigzag64(int64_t v) {
        return (v << 1) ^ (v >> 63);
    }

    static int32_t DecodeZigzag32(uint32_t v) {
        return (v >> 1) ^ -(v & 1);
    }

    static int64_t DecodeZigzag64(uint64_t v) {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32(int32_t value) {
        writeUint32(EncodeZigzag32(value));
    }

    void ByteArray::writeUint32(uint32_t value) {
        uint8_t tmp[5];
        size_t i = 0;
        while (value >= 0x80) {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeInt64(int64_t value) {
        writeUint64(EncodeZigzag64(value));
    }

    void ByteArray::writeUint64(uint64_t value) {
        uint8_t tmp[10];
        size_t i = 0;
        while (value >= 0x80) {
            tmp[i++] = (value & 0x7f) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeFloat(float value) {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint32(v);
    }

    void ByteArray::writeDouble(double value) {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint64(v);
    }

    void ByteArray::writeStringF16(const std::string& value) {
        writeFuint16(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF32(const std::string& value) {
        writeFuint32(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringF64(const std::string& value) {
        writeFuint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringVint(const std::string& value) {
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string& value) {
        write(value.c_str(), value.size());
    }

    int8_t ByteArray::readFint8() {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFuint8() {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

    int16_t ByteArray::readFint16() {
        int16_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint16_t ByteArray::readFuint16() {
        uint16_t v;
        read(&v, sizeof(v));
        return v;
    }

    int32_t ByteArray::readFint32() {
        int32_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint32_t ByteArray::readFuint32() {
        uint32_t v;
        read(&v, sizeof(v));
        return v;
    }

    int64_t ByteArray::readFint64() {
        int64_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint64_t ByteArray::readFuint64() {
        uint64_t v;
        read(&v, sizeof(v));
        return v;
    }

    int32_t ByteArray::readInt32() {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32() {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7) {
            uint8_t b = readFuint8();
            result |= (((uint32_t)(b & 0x7f)) << i);
            if (b < 0x80) {
                break;
            }
        }
        return result;
    }

    int64_t ByteArray::readInt64() {
        return DecodeZigzag64(readUint64());
    }

    uint64_t ByteArray::readUint64() {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7) {
            uint8_t b = readFuint8();
            result |= (((uint64_t)(b & 0x7f)) << i);
            if (b < 0x80) {
                break;
            }
        }
        return result;
    }

    float ByteArray::readFloat() {
        uint32_t v = readFuint32();
        float result;
        memcpy(&result, &v, sizeof(v));
        return result;
    }

    double ByteArray::readDouble() {
        uint64_t v = readFuint32();
        double result;
        memcpy(&result, &v, sizeof(v));
        return result;
    }

    std::string ByteArray::readStringF16() {
        uint16_t len = readFuint16();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::readStringF32() {
        uint32_t len = readFuint32();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::readStringF64() {
        uint64_t len = readFuint64();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    std::string ByteArray::StringVint() {
        uint64_t len = readUint64();
        std::string buf;
        buf.resize(len);
        read(&buf[0], len);
        return buf;
    }

    void ByteArray::clear() {
        m_position = 0;
        m_size = 0;
        Node* tmp = m_root->next;
        while (tmp) {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = nullptr;
    }

    void ByteArray::write(const void* buf, size_t size) {
        if (size == 0) {
            return;
        }
        addCapacity(size);
        size_t npos = m_position % m_baseSize;     // 写入内存块的位置
        size_t ncap = m_cur->size - npos;          // 当前内存块的容量
        size_t bpos = 0;                           // 已写入数据长度
        while (size > 0) {
            if (ncap >= size) {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
                if (m_cur->size == npos + size) {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
                m_position += ncap;
                size -= ncap;
                bpos += ncap;
                m_cur = m_cur->next;
                npos = 0;
                ncap = m_cur->size;
            }
        }
        if (m_position > m_size) {
            m_size = m_position;
        }
    }

    void ByteArray::read(void* buf, size_t size) {
        if (size > getReadSize()) {
            throw std::out_of_range("not enough len");
        }
        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        size_t bpos = 0;
        while (size > 0) {
            if (ncap >= size) {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
                if (m_cur->size == npos + size) {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
                m_position += ncap;
                size -= ncap;
                bpos += ncap;
                m_cur = m_cur->next;
                npos = 0;
                ncap = m_cur->size;
            }
        }
    }

    void ByteArray::read(void* buf, size_t size, size_t position) const {
        if (size > getReadSize()) {
            throw std::out_of_range("not enough len");
        }
        Node* cur = m_cur;
        size_t npos = position % m_baseSize;
        size_t ncap = cur->size - npos;
        size_t bpos = 0;
        while (size > 0) {
            if (ncap >= size) {
                memcpy((char*)buf + bpos, cur->ptr + npos, size);
                if (cur->size == npos + size) {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            } else {
                memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
                position += ncap;
                size -= ncap;
                bpos += ncap;
                cur = cur->next;
                npos = 0;
                ncap = cur->size;
            }
        }
    }

    bool ByteArray::writeToFile(const std::string& name) const {
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary);
        if (!ofs) {
            SYLAR_LOG_ERROR(g_logger) << "writeToFile name=" << name
                << " error, errno=" << errno << ", errstr=" << strerror(errno);
            return false;
        }
        size_t readSize = getReadSize();
        size_t pos = m_position;
        Node* cur = m_cur;
        while (readSize > 0) {
            pos %= m_baseSize;
            size_t len = (readSize + pos > m_baseSize ? m_baseSize - pos : readSize);
            ofs.write(cur->ptr + pos, len);
            cur = cur->next;
            pos += len;
            readSize -= len;
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string& name) {
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);
        if (!ifs) {
            SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
                << " error, errno=" << errno << ", errstr=" << strerror(errno);
            return false;
        }
        std::shared_ptr<char[]> buff(new char[m_baseSize]);
        while (!ifs.eof()) {
            ifs.read(buff.get(), m_baseSize);
            write(buff.get(), ifs.gcount());
        }
        return true;
    }

    std::string ByteArray::toString() const {
        std::string str;
        str.resize(getReadSize());
        if (str.empty()) {
            return str;
        }
        read(&str[0], str.size(), m_position);
        return str;
    }

    std::string ByteArray::toHexString() const {
        std::string str = toString();
        std::stringstream ss;
        for (size_t i = 0; i < str.size(); ++i) {
            if (i > 0 && i % 32 == 0) {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex << (uint8_t)str[i] << " ";
        }
        return ss.str();
    }

    void ByteArray::setPosition(size_t v) {
        if (v > m_capacity) {
            throw std::out_of_range("setPosition out of range");
        }
        m_position = v;
        if (m_position > m_size) {
            m_size = m_position;
        }
        m_cur = m_root;
        while (v >= m_cur->size) {
            v -= m_cur->size;
            m_cur = m_cur->next;
        }
    }

    void ByteArray::addCapacity(size_t size) {
        if (size == 0) {
            return;
        }
        size_t oldCap = getCapacity();
        if (oldCap >= size) {
            return;
        }
        size_t count = (size + m_baseSize - 1) / m_baseSize;
        Node* tmp = m_root;
        while (tmp->next) {
            tmp = tmp->next;
        }
        Node* first = nullptr;
        for (size_t i = 0; i < count; ++i) {
            tmp->next = new Node(m_baseSize);
            if (first == nullptr) {
                first = tmp->next;
            }
            tmp = tmp->next;
            m_capacity += m_baseSize;
        }
        if (oldCap == 0) {
            m_cur = first;
        }
    }



}