#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <string>
#include <set>
#include <memory>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <yaml-cpp/yaml.h>
#include "log.h"

namespace sylar
{
    template<typename F, typename T>
    class LexicalCast
    {
    public:
        T operator()(const F& v) {
            return boost::lexical_cast<T>(v);
        }
    };

    template<typename T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            std::vector<T> vec;
            std::stringstream ss;
            for (auto it : node) {
                ss.str("");
                ss << it;
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<typename T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T>& vec) {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto& t : vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(t)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template<typename T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            std::list<T> list;
            std::stringstream ss;
            for (auto it : node) {
                ss.str("");
                ss << it;
                list.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return list;
        }
    };

    template<typename T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T>& list) {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto& t : list) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(t)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template<typename T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            std::set<T> set;
            std::stringstream ss;
            for (auto it : node) {
                ss.str("");
                ss << it;
                set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return set;
        }
    };

    template<typename T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T>& set) {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto& t : set) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(t)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template<typename T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            std::map<std::string, T> map;
            std::stringstream ss;
            for (auto it : node) {
                ss.str("");
                ss << it.second;
                map.insert({ it.first.Scalar(), LexicalCast<std::string, T>()(ss.str()) });
            }
            return map;
        }
    };

    template<typename T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T>& map) {
            YAML::Node node(YAML::NodeType::Map);
            for (auto& i : map) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}

namespace sylar
{
    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;

        ConfigVarBase(const std::string& name, const std::string& description = "")
            : m_name(name), m_description(description) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), tolower);
        }
        virtual ~ConfigVarBase() {}
        const std::string& getName() const { return m_name; }
        const std::string& getDescripton() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& str) = 0;
        virtual std::string getTypeName() const = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };

    template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar>;
        using on_change_cb = std::function<void(const T& old_value, const T& new_value)>;
        ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
            : ConfigVarBase(name, description), m_val(default_value) {}

        std::string toString() override {
            try {
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            } catch (std::exception& e) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toStirng exception"
                    << e.what() << " convert: " << getTypeName() << " to string";
            }
            return "";
        }

        bool fromString(const std::string& str) override {
            try {
                // m_val = boost::lexical_cast<T>(str);
                setValue(FromStr()(str));
                return true;
            } catch (std::exception& e) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConFigVar::fromString exception"
                    << e.what() << " convert: string to " << getTypeName();
            }
            return false;
        }

        std::string getTypeName() const override { return typeid(T).name(); }

        const T getValue() const { return m_val; }

        void setValue(const T& val) {
            if (m_val == val) {
                return;
            }
            for (auto& it : m_cbs) {
                it.second(m_val, val);
            }
            m_val = val;
        }

        uint64_t addListener(on_change_cb cb) {
            static uint64_t s_fun_id = 0;
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        void delListener(uint64_t key) {
            m_cbs.erase(key);
        }

        on_change_cb getListener(uint64_t key) {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        void clearListener() {
            m_cbs.clear();
        }

    private:
        T m_val;
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    class Config
    {
    public:
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

        template<typename T>
        static typename ConfigVar<T>::ptr Add(const std::string& name, const T& default_value, const std::string& description = "") {
            auto it = GetDatas().find(name);
            if (it != GetDatas().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Add name=" << name << " exists.";
                    return tmp;
                } else {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Add name=" << name << " exists but type not "
                        << typeid(T).name() << ", real_type=" << it->second->getTypeName();
                }
            }
            // auto tmp = Lookup<T>(name);
            // if (tmp) {
            //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists.";
            //     return tmp;
            // }
            if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }
            typename ConfigVar<T>::ptr v = std::make_shared<ConfigVar<T>>(name, default_value, description);
            GetDatas()[name] = v;
            return v;
        }

        // template<typename T>
        // static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        //     auto it = s_datas.find(name);
        //     if (it == s_datas.end()) {
        //         return nullptr;
        //     }
        //     return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        // }

        static void LoadFromYaml(const YAML::Node& root);

        static ConfigVarBase::ptr LookupBase(const std::string& name) {
            auto it = GetDatas().find(name);
            if (it == GetDatas().end()) {
                return nullptr;
            }
            return it->second;
        }

    private:
        static ConfigVarMap& GetDatas() {
            static ConfigVarMap s_datas;
            return s_datas;
        }

        // static ConfigVarMap s_datas;
    };

}


#endif