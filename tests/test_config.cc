#include "../sylar/log.h"
#include <yaml-cpp/yaml.h>
#include "../sylar/config.h"


void print_yaml(YAML::Node& node, int level) {
    if (node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar()
            << " - " << node.Type() << " - " << level << " - scalar";
    } else if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - "
            << node.Type() << " - " << level << " - null";
    } else if (node.IsMap()) {
        for (auto it : node) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it.first
                << " - " << it.second.Type() << " - " << level << " - map";
            print_yaml(it.second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (auto it : node) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it
                << " - " << it.Type() << " - " << level << " - seq";
            print_yaml(it, level + 1);
        }
    }
}


void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/tang/workspaces/sylar/bin/conf/log.yml");
    print_yaml(root, 0);
}

int main(int argc, char** argv) {

    sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");
    
    sylar::ConfigVar<float>::ptr g_float_value_config = sylar::Config::Lookup("system.port", (float)10.25, "system value");

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_value_config->toString();

    // test_yaml();
}