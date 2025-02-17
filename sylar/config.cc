#include "config.h"
#include "env.h"
#include "util.h"
#include "log.h"

#include <sys/stat.h>

namespace sylar
{
    // Config::ConfigVarMap Config::s_datas;

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static void ListAllMember(const std::string& prefix, const YAML::Node& node,
        std::list<std::pair<std::string, const YAML::Node>>& output) {
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back({ prefix, node });
        if (node.IsMap()) {
            for (auto it : node) {
                ListAllMember(prefix.empty() ? it.first.Scalar()
                    : prefix + "." + it.first.Scalar(), it.second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node& root) {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for (auto& it : all_nodes) {
            std::string key = it.first;
            if (key.empty()) {
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), tolower);
            ConfigVarBase::ptr var = LookupBase(key);
            if (var) {
                if (it.second.IsScalar()) {
                    var->fromString(it.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << it.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    static std::map<std::string, uint64_t> s_file2modifytime;
    static Mutex s_mutex;

    void Config::LoadFromConfDir(const std::string& path, bool force) {
        std::string absoulte_path = sylar::EnvMgr::GetInstance()->getAbsoluetPath(path);
        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absoulte_path, ".yml");
        for (auto& it : files) {
            {
                struct stat st;
                lstat(it.c_str(), &st);
                Mutex::Lock lock(s_mutex);
                if (!force && s_file2modifytime[it] == (uint64_t)st.st_mtime) {
                    continue;
                }
                s_file2modifytime[it] = st.st_mtime;
            }
            try {
                YAML::Node root = YAML::LoadFile(it);
                LoadFromYaml(root);
                SYLAR_LOG_INFO(g_logger) << "LoadConfFile file=" << it << " ok";
            } catch (...) {
                SYLAR_LOG_ERROR(g_logger) << "LoadconfFile file=" << it << " failed";
            }
        }
    }


}