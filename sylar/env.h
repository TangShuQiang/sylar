#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__

#include <map>
#include <vector>

#include "singleton.h"
#include "thread.h"

namespace sylar
{
    class Env
    {
    public:
        using RWMutexType = RWMutex;

        bool init(int argc, char** argv);

        void add(const std::string& key, const std::string& val);
        bool has(const std::string& key);
        void del(const std::string& key);
        std::string get(const std::string& key, const std::string& default_val = "");

        void addHelp(const std::string& key, const std::string& desc);
        void removeHelp(const std::string& key);
        void printHelp();

        const std::string& getExe() const { return m_exe; }
        const std::string& getCwd() const { return m_cwd; }

        bool setEnv(const std::string& key, const std::string& val);
        std::string getEnv(const std::string& key, const std::string& default_val = "");

        std::string getAbsoluetPath(const std::string& path) const;
        std::string getAbsoluetWorkPath(const std::string& path) const;
        std::string getConfigPath();

    private:
        RWMutexType m_mutex;
        std::map<std::string, std::string> m_args;
        std::vector<std::pair<std::string, std::string>> m_helps;

        std::string m_paogram;
        std::string m_exe;
        std::string m_cwd;
    };

    using EnvMgr = sylar::Singleton<Env>;
}

#endif