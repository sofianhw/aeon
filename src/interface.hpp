/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include <exception>
#include <string>
#include <sstream>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

#include "typemap.hpp"
#include "util.hpp"
#include "json.hpp"

static int IGNORE_VALUE;

namespace nervana
{
    namespace interface
    {
        class config_info_interface;
        template <typename>
        class config_info;
        class config;
        template <typename T>
        class extractor;
        template <typename T, typename S>
        class transformer;
        template <typename T, typename S>
        class param_factory;
        template <typename T>
        class loader;
        class decoded_media;
        class params;
    }
    typedef std::vector<size_t> shape_t;

    std::string dump_default(const std::string& s);
    std::string dump_default(int v);
    std::string dump_default(uint32_t v);
    std::string dump_default(size_t v);
    std::string dump_default(float v);
    std::string dump_default(const std::vector<float>& v);
    std::string dump_default(const std::vector<std::string>& v);
    std::string dump_default(const std::uniform_real_distribution<float>& v);
    std::string dump_default(const std::uniform_int_distribution<int>& v);
    std::string dump_default(const std::normal_distribution<float>& v);
    std::string dump_default(const std::bernoulli_distribution& v);
}

class nervana::interface::config_info_interface
{
public:
    virtual const std::string& name() const       = 0;
    virtual void parse(nlohmann::json js)         = 0;
    virtual bool        required() const          = 0;
    virtual std::string type() const              = 0;
    virtual std::string get_default_value() const = 0;
};

class nervana::interface::config
{
public:
    config() {}
    const nervana::shape_type& get_shape_type(size_t index = 0) const
    {
        if (index >= m_shape_type_list.size())
            throw std::out_of_range("config output shape index out of range");
        return m_shape_type_list[index];
    }
    const std::vector<nervana::shape_type>& get_shape_type_list() const
    {
        return m_shape_type_list;
    }
    void verify_config(const std::string& location,
                       const std::vector<std::shared_ptr<interface::config_info_interface>>& config,
                       nlohmann::json js) const;

    enum class mode
    {
        OPTIONAL,
        REQUIRED
    };

#define ADD_SCALAR(var, mode, ...)                                                                 \
    std::make_shared<nervana::interface::config_info<decltype(var)>>(                              \
        var, #var, mode, parse_value<decltype(var)>, ##__VA_ARGS__)
#define ADD_IGNORE(var)                                                                            \
    std::make_shared<nervana::interface::config_info<int>>(                                        \
        IGNORE_VALUE,                                                                              \
        #var,                                                                                      \
        mode::OPTIONAL,                                                                            \
        [](int, const std::string&, const nlohmann::json&, mode) {})
#define ADD_DISTRIBUTION(var, mode, ...)                                                           \
    std::make_shared<nervana::interface::config_info<decltype(var)>>(                              \
        var, #var, mode, parse_dist<decltype(var)>, ##__VA_ARGS__)

    template <typename T, typename S>
    static void set_dist_params(T& dist, S& params)
    {
        dist = T{params[0], params[1]};
    }

    // Specialization for a bernoulli coin flipping random var
    static void set_dist_params(std::bernoulli_distribution& dist, std::vector<bool>& params)
    {
        dist = std::bernoulli_distribution{params[0] ? 0.5 : 0.0};
    }

    template <typename T>
    static void parse_dist(T&                    value,
                           const std::string&    key,
                           const nlohmann::json& js,
                           mode                  required = mode::OPTIONAL)
    {
        auto val = js.find(key);
        if (val != js.end())
        {
            auto params = val->get<std::vector<typename T::result_type>>();
            set_dist_params(value, params);
        }
    }

    template <typename T>
    static void parse_value(T&                    value,
                            const std::string&    key,
                            const nlohmann::json& js,
                            mode                  required = mode::OPTIONAL)
    {
        auto val = js.find(key);
        if (val != js.end())
        {
            value = val->get<T>();
        }
        else if (required == mode::REQUIRED)
        {
            throw std::invalid_argument("Required Argument: " + key + " not set");
        }
    }

    template <typename T>
    static void parse_enum(T&                    value,
                           const std::string     key,
                           const nlohmann::json& js,
                           mode                  required = mode::OPTIONAL)
    {
        auto val = js.find(key);
        if (val != js.end())
        {
            std::string tmp = val->get<std::string>();
            from_string(value, tmp);
        }
        else if (required == mode::REQUIRED)
        {
            throw std::invalid_argument("Required Argument: " + key + " not set");
        }
    }

    void add_shape_type(const std::vector<size_t>& sh,
                        const std::string&         output_type,
                        const bool                 flatten_all_dims = false)
    {
        m_shape_type_list.emplace_back(sh, nervana::output_type{output_type}, flatten_all_dims);
    }
    void add_shape_type(const std::vector<size_t>&  sh,
                        const nervana::output_type& ot,
                        const bool                  flatten_all_dims = false)
    {
        m_shape_type_list.emplace_back(sh, ot, flatten_all_dims);
    }

private:
    std::vector<nervana::shape_type> m_shape_type_list;
};

namespace nervana
{
    template <class T>
    std::string type_name()
    {
        typedef typename std::remove_reference<T>::type TR;
        std::unique_ptr<char, void (*)(void*)> own(
#ifndef _MSC_VER
            abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
#else
            nullptr,
#endif
            std::free);
        std::string r = own != nullptr ? own.get() : typeid(TR).name();
        if (std::is_const<TR>::value)
            r += " const";
        if (std::is_volatile<TR>::value)
            r += " volatile";
        if (std::is_lvalue_reference<T>::value)
            r += "&";
        else if (std::is_rvalue_reference<T>::value)
            r += "&&";
        return r;
    }
}

template <typename T>
class nervana::interface::config_info : public nervana::interface::config_info_interface
{
public:
    config_info(
        T&                               var,
        const std::string&               name,
        nervana::interface::config::mode m,
        std::function<void(
            T&, const std::string&, const nlohmann::json&, nervana::interface::config::mode)> parse,
        std::function<bool(T)> validate = [](T) -> bool { return true; })
        : m_target_variable{var}
        , m_variable_name{name}
        , m_parse_mode{m}
        , m_parse_function{parse}
        , m_validate_function{validate}
        , m_default_value{var}
    {
    }

    const std::string& name() const override { return m_variable_name; }
    bool required() const override { return m_parse_mode == interface::config::mode::REQUIRED; }
    std::string type() const override { return type_name<T>(); }
    std::string get_default_value() const override { return dump_default(m_default_value); }
    void parse(nlohmann::json js) override
    {
        m_parse_function(m_target_variable, m_variable_name, js, m_parse_mode);
        if (!m_validate_function(m_target_variable))
        {
            std::stringstream ss;
            ss << "value for '" << m_variable_name << "' out of range";
            throw std::invalid_argument(ss.str());
        }
    }

private:
    config_info() = delete;
    T&                               m_target_variable;
    const std::string                m_variable_name;
    nervana::interface::config::mode m_parse_mode;
    std::function<void(
        T&, const std::string&, const nlohmann::json&, nervana::interface::config::mode)>
                           m_parse_function;
    std::function<bool(T)> m_validate_function;
    T                      m_default_value;
};

template <typename T, typename S>
class nervana::interface::param_factory
{
public:
    virtual ~param_factory() {}
    virtual std::shared_ptr<S> make_params(std::shared_ptr<const T>) = 0;
};

template <typename T>
class nervana::interface::extractor
{
public:
    virtual ~extractor() {}
    virtual std::shared_ptr<T> extract(const void*, size_t) = 0;
};

template <typename T, typename S>
class nervana::interface::transformer
{
public:
    virtual ~transformer() {}
    virtual std::shared_ptr<T> transform(std::shared_ptr<S>, std::shared_ptr<T>) = 0;
};

template <typename T>
class nervana::interface::loader
{
public:
    virtual ~loader() {}
    virtual void load(const std::vector<void*>&, std::shared_ptr<T>) = 0;
};

/*  ABSTRACT INTERFACES */
class nervana::interface::decoded_media
{
public:
    virtual ~decoded_media() {}
};

/*  ABSTRACT INTERFACES */
class nervana::interface::params
{
public:
    virtual ~params() {}
};
