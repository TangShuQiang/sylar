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

    sylar::ConfigVar<std::list<int> >::ptr g_int_list_value_config = sylar::Config::Add("system.int_list", std::list<int>{3, 4}, "system int list");

    sylar::ConfigVar<std::set<int> >::ptr g_int_set_value_config = sylar::Config::Add("system.int_set", std::set<int>{3, 4, 2, 3, 5}, "system int set");

    sylar::ConfigVar<std::map<std::string, int> >::ptr g_int_map_value_config = sylar::Config::Add("system.int_map", std::map<std::string, int>{{"k1", 20}}, "system int map");

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->getValue();

    auto v = g_int_vec_value_config->getValue();
    for (auto it : v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int_vec: " << it;
    }

    auto v_list = g_int_list_value_config->getValue();
    for (auto it : v_list) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int_list: " << it;
    }

    auto v_set = g_int_set_value_config->getValue();
    for (auto it : v_set) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int_set: " << it;
    }

    auto v_map = g_int_map_value_config->getValue();
    for (auto it : v_map) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before v_map: " << it.first << "-" << it.second;
    }

    YAML::Node root = YAML::LoadFile("/home/tang/workspaces/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    v = g_int_vec_value_config->getValue();
    for (auto it : v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int_vec: " << it;
    }
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "int_vec toString: " << g_int_vec_value_config->toString();

    v_list = g_int_list_value_config->getValue();
    for (auto it : v_list) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int_list: " << it;
    }
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "int_list toString: " << g_int_list_value_config->toString();

    v_set = g_int_set_value_config->getValue();
    for (auto it : v_set) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int_set: " << it;
    }
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "int_set toString: " << g_int_set_value_config->toString();

    v_map = g_int_map_value_config->getValue();
    for (auto it : v_map) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after v_map: " << it.first << "-" << it.second;
    }
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "int_map toString: " << g_int_map_value_config->toString();

}

void test_same_name_config() {
    sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Add("system.port", (int)8080, "system port");

    sylar::ConfigVar<float>::ptr g_float_value_config = sylar::Config::Add("system.port", (float)10.25, "system value");
}


class Person
{
public:
    Person() {}
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
            << " age=" << m_age
            << " sex=" << m_sex
            << "]";
        return ss.str();
    }

    bool operator==(const Person& rhs) const {
        return m_name == rhs.m_name
            && m_age == rhs.m_age
            && m_sex == rhs.m_sex;
    }
};

namespace sylar
{
    template<>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template<>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person& p) {
            YAML::Node node(YAML::NodeType::Map);
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

void test_class() {
    sylar::ConfigVar<Person>::ptr conf_person_val = sylar::Config::Add("system.person", Person(), "person");

    conf_person_val->addListener([](const Person& old_val, const Person& new_val) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_val: " << old_val.toString();
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "new_val: " << new_val.toString();
    });

    Person p = conf_person_val->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << p.toString();

    YAML::Node root = YAML::LoadFile("/home/tang/workspaces/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    p = conf_person_val->getValue();

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << p.toString();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << conf_person_val->toString();
}

int main(int argc, char** argv) {
    // test_config();
    // test_same_name_config();
    test_class();

    // test_yaml();
}