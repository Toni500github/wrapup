#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0

#include "toml++/toml.hpp"
#include "util.hpp"

class Config
{
public:
    Config(const std::string_view configFile, const std::string_view configDir);

    std::string clr_title;
    std::string clr_description;
    std::string clr_example_text;
    std::string clr_example_code;

private:
    void        loadConfigFile(const std::string_view filename);
    void        generateConfig(const std::string_view filename);
    std::string getThemeValue(const std::string_view value, const std::string_view fallback) const;

    template <typename T>
    T getValue(const std::string_view value, const T&& fallback) const
    {
        std::optional<T> ret = this->tbl.at_path(value).value<T>();
        if constexpr (toml::is_string<T>)  // if we want to get a value that's a string
            return ret ? expandVar(ret.value()) : expandVar(fallback);
        else
            return ret.value_or(fallback);
    }

    toml::table tbl;
};

inline constexpr std::string_view AUTOCONFIG = R"#([colors]
title = "\e[1m"
description = "\e[34m"
example-text = "\e[36m"
example-code = "\e[33m"

)#";

#endif
