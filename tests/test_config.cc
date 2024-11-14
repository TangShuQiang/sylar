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

void test_config() {
    sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Add("system.port", (int)8080, "system port");

    sylar::ConfigVar<float>::ptr g_float_value_config = sylar::Config::Add("system.value", (float)10.25, "system value");

    sylar::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = sylar::Config::Add("system.int_vec", std::vector<int>{1, 2}, "system int vec");

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    auto v = g_int_vec_value_config->getValue();
    for (auto it : v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int_vec: " << it;
    }

    YAML::Node root = YAML::LoadFile("/home/tang/workspaces/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    v = g_int_vec_value_config->getValue();
    for (auto it : v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int_vec: " << it;
    }

}

int main(int argc, char** argv) {
    test_config();

    // test_yaml();
}