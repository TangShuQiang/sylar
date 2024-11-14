#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <string>
#include <memory>
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
    protected:
        std::string m_name;
        std::string m_description;
    };

    template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar>;
        ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
            : ConfigVarBase(name, description), m_val(default_value) {}

        std::string toString() override {
            try {
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            } catch (std::exception& e) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toStirng exception"
                    << e.what() << " convert: " << typeid(m_val).name() << " to string";
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
                    << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { return m_val; }
        void setValue(const T& val) { m_val = val; }
    private:
        T m_val;
    };

    class Config
    {
    public:
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

        template<typename T>
        static typename ConfigVar<T>::ptr Add(const std::string& name, const T& default_value, const std::string& description = "") {
            auto tmp = Lookup<T>(name);
            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists.";
                return tmp;
            }
            if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789") != std::string::npos) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }
            typename ConfigVar<T>::ptr v = std::make_shared<ConfigVar<T>>(name, default_value, description);
            s_datas[name] = v;
            return v;
        }

        template<typename T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
            auto it = s_datas.find(name);
            if (it == s_datas.end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node& root);

        static ConfigVarBase::ptr LookupBase(const std::string& name) {
            auto it = s_datas.find(name);
            if (it == s_datas.end()) {
                return nullptr;
            }
            return it->second;
        }

    private:
        static ConfigVarMap s_datas;
    };

}


#endif