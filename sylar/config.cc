#include "config.h"

namespace sylar
{
    // Config::ConfigVarMap Config::s_datas;

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

}