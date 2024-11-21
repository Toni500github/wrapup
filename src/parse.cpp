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

#include "parse.hpp"

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>
#include "fmt/base.h"
#include "fmt/color.h"
#include "util.hpp"

// this is why I unironically like C/C++ being OS depended
std::string get_platform()
{
#if (defined(__ANDROID__) || defined(ANDROID_API))
    return "android";

#elif (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__))
    return "windows";

#elif (defined (__APPLE__) || defined(__OSX__))
    return "osx";

#elif (defined (__sun) || defined(__SunOS) || defined(sun))
    return "sunos";

#elif (defined(linux) || defined(__linux) || defined(__linux__))
    return "linux";

#elif (defined(__OpenBSD__))
    return "openbsd";

#elif (defined(__NetBSD__))
    return "netbsd"

#elif (defined(__FreeBSD__))
    return "freebsd";
#endif
    
    return "common";
}

void parse_page(const std::string_view page)
{
    std::string path = fmt::format("{}/pages/{}/{}", getCacheDir(), get_platform(), page);
    debug("path = {}", path);
    std::fstream f(path);
    if (!f.is_open())
    {
        path = fmt::format("{}/pages/common/{}", getCacheDir(), page);
        f.open(path);
        if (!f.is_open())
            die("failed to open {}", path);
    }

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty())
            continue;

        switch (line.front())
        {
            case '#': line.replace(0, 1, NOCOLOR_BOLD); fmt::print("\n\n"); break;
            case '>': line.replace(0, 1, "\033[34m"); break;
            case '-':
                line.replace(1, 1, "\033[36m ");
                fmt::print("\n");
                break;
            case '`':
                line.replace(0, 1, "\033[33m");
                line.replace(line.find('`', 2), 1, NOCOLOR);
                size_t pos = 0;
                while ((pos = line.find("{{")) != line.npos)
                {
                    line.replace(pos, 2, NOCOLOR);
                    pos = line.find("}}", pos);
                    if (pos != line.npos)
                    line.replace(pos, 2, "\033[33m");
                }
                fmt::println("  \t{}\033[0m", line);
                continue;
        }

        fmt::println("  {}\033[0m", line);
    }
    fmt::print("\n\n");
}
