/*
 * Copyright 2024 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <dlfcn.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <vector>

#include "fmt/color.h"
#include "fmt/core.h"

// clang-format off
constexpr std::size_t operator""_len(const char*, std::size_t ln) noexcept
{ 
    return ln;
}

constexpr const char NOCOLOR[] = "\033[0m";
constexpr const char NOCOLOR_BOLD[] = "\033[0m\033[1m";
constexpr const char UNKNOWN[] = "(unknown)";

bool         hasEnding(const std::string_view fullString, const std::string_view ending);
bool         hasStart(const std::string_view fullString, const std::string_view start);
std::string  read_shell_exec(const std::string_view cmd);
void         ctrl_d_handler(const std::istream& cin);
std::string  expandVar(std::string ret);
bool         taur_exec(const std::vector<std::string_view> cmd_str, const bool noerror_print = true);
std::string  which(const std::string_view command);
std::string  get_data_path(const std::string_view file);
std::string  get_data_dir(const std::string_view dir);
std::string  get_relative_path(const std::string_view relative_path, const std::string_view _env, const long long mode);
void         replace_str(std::string& str, const std::string_view from, const std::string_view to);
bool         read_exec(std::vector<const char*> cmd, std::string& output, bool useStdErr = false, bool noerror_print = true);
std::string  str_tolower(std::string str);
std::string  str_toupper(std::string str);
fmt::rgb     hexStringToColor(const std::string_view hexstr);
std::string  getHomeCacheDir();
std::string  getCacheDir();
std::string  getHomeConfigDir();
std::string  getConfigDir();
std::vector<std::string> split(const std::string_view text, char delim);


#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))
template <typename... Args>
void error(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "ERROR: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void die(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(stderr, BOLD_COLOR(fmt::rgb(fmt::color::red)), "FATAL: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    std::exit(1);
}

template <typename... Args>
void debug(const std::string_view fmt, Args&&... args) noexcept
{
#if DEBUG
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::hot_pink))), "[DEBUG]: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
#endif
}

template <typename... Args>
void warn(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::yellow))), "WARNING: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) noexcept
{
    fmt::print(BOLD_COLOR((fmt::rgb(fmt::color::cyan))), "INFO: {}\n",
                 fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
}

/** Ask the user a yes or no question.
 * @param def The default result
 * @param fmt The format string
 * @param args Arguments in the format
 * @returns the result, y = true, n = false, only returns def if the result is def
 */
template <typename... Args>
bool askUserYorN(bool def, const std::string_view fmt, Args&&... args)
{
    const std::string& inputs_str = fmt::format(" [{}]: ", def ? "Y/n" : "y/N");
    std::string result;
    fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
    fmt::print("{}", inputs_str);

    while (std::getline(std::cin, result) && (result.length() > 1))
        fmt::print(BOLD_COLOR(fmt::rgb(fmt::color::yellow)), "Please answear y or n,{}", inputs_str);

    ctrl_d_handler(std::cin);

    if (result.empty())
        return def;

    if (def ? std::tolower(result[0]) != 'n' : std::tolower(result[0]) != 'y')
        return def;

    return !def;
}

#endif
