#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar
{
    template<typename T>
    class Singleton
    {
    public:
        static T* GetInstance() {
            static T v;
            return &v;
        }
    private:
        Singleton() {};
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
    };

    template<typename T>
    class SingletonPtr
    {
    public:
        static std::shared_ptr<T> GetInstance() {
            static std::shared_ptr<T> v = std::make_shared<T>();
            return v;
        }
    private:
        SingletonPtr() {};
        SingletonPtr(const SingletonPtr&) = delete;
        SingletonPtr& operator=(const SingletonPtr&) = delete;
    };
}

#endif