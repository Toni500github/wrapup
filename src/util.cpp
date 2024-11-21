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

#include "util.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/color.h"
#include "fmt/ranges.h"

// https://stackoverflow.com/questions/874134/find-out-if-string-ends-with-another-string-in-c#874160
bool hasEnding(const std::string_view fullString, const std::string_view ending)
{
    if (ending.length() > fullString.length())
        return false;
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
}

bool hasStart(const std::string_view fullString, const std::string_view start)
{
    if (start.length() > fullString.length())
        return false;
    return (fullString.substr(0, start.size()) == start);
}

std::vector<std::string> split(const std::string_view text, const char delim)
{
    std::string              line;
    std::vector<std::string> vec;
    std::stringstream        ss(text.data());
    while (std::getline(ss, line, delim))
    {
        vec.push_back(line);
    }

    return vec;
}

void ctrl_d_handler(const std::istream& cin)
{
    if (cin.eof())
        die("Exiting due to CTRL-D or EOF");
}

/** Replace special symbols such as ~ and $ in std::strings
 * @param str The string
 * @return The modified string
 */
std::string expandVar(std::string ret)
{
    if (ret.empty())
        return ret;

    const char* env;
    if (ret.front() == '~')
    {
        env = std::getenv("HOME");
        if (env == nullptr)
            die("FATAL: $HOME enviroment variable is not set (how?)");

        ret.replace(0, 1, env);  // replace ~ with the $HOME value
    }
    else if (ret.front() == '$')
    {
        ret.erase(0, 1);

        std::string   temp;
        const size_t& pos = ret.find('/');
        if (pos != std::string::npos)
        {
            temp = ret.substr(pos);
            ret.erase(pos);
        }

        env = std::getenv(ret.c_str());
        if (env == nullptr)
            die("No such enviroment variable: {}", ret);

        ret = env;
        ret += temp;
    }

    return ret;
}

/**
 * remove all white spaces (' ', '\t', '\n') from start and end of input
 * inplace!
 * @param input
 * @Original https://github.com/lfreist/hwinfo/blob/main/include/hwinfo/utils/stringutils.h#L50
 */
void strip(std::string& input)
{
    if (input.empty())
    {
        return;
    }

    // optimization for input size == 1
    if (input.size() == 1)
    {
        if (input.at(0) == ' ' || input.at(0) == '\t' || input.at(0) == '\n')
        {
            input = "";
            return;
        }
        else
        {
            return;
        }
    }

    // https://stackoverflow.com/a/25385766
    const char* ws = " \t\n\r\f\v";
    input.erase(input.find_last_not_of(ws) + 1);
    input.erase(0, input.find_first_not_of(ws));
}

fmt::rgb hexStringToColor(const std::string_view hexstr)
{
    std::stringstream ss;
    ss << std::hex << hexstr.substr(1).data();

    uint value;
    ss >> value;

    return fmt::rgb(value);
}

std::string get_relative_path(const std::string_view relative_path, const std::string_view _env, const long long mode)
{
    const char *env = std::getenv(_env.data());
    if (!env)
        return UNKNOWN;

    struct stat sb;
    std::string fullPath;
    fullPath.reserve(1024);

    for (const std::string& dir : split(env, ':'))
    {
        // -300ns for not creating a string. stonks
        fullPath += dir;
        fullPath += '/';
        fullPath += relative_path.data();
        if ((stat(fullPath.c_str(), &sb) == 0) && sb.st_mode & mode)
            return fullPath.c_str();

        fullPath.clear();
    }

    return UNKNOWN;  // not found
}

std::string which(const std::string_view command)
{
    return get_relative_path(command, "PATH", S_IXUSR);
}

std::string get_data_path(const std::string_view file)
{
    return get_relative_path(file, "XDG_DATA_DIRS", S_IFREG);
}

std::string get_data_dir(const std::string_view dir)
{
    return get_relative_path(dir, "XDG_DATA_DIRS", S_IFDIR);
}

// https://gist.github.com/GenesisFR/cceaf433d5b42dcdddecdddee0657292
void replace_str(std::string& str, const std::string_view from, const std::string_view to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Handles case where 'to' is a substring of 'from'
    }
}

bool read_exec(std::vector<const char*> cmd, std::string& output, bool useStdErr, bool noerror_print)
{
    debug("{} cmd = {}", __func__, cmd);
    std::array<int, 2> pipeout;

    if (pipe(pipeout.data()) < 0)
        die("pipe() failed: {}", strerror(errno));

    const pid_t pid = fork();

    // we wait for the command to finish then start executing the rest
    if (pid > 0)
    {
        close(pipeout.at(1));

        int status;
        waitpid(pid, &status, 0);  // Wait for the child to finish

        if (WIFEXITED(status) && (WEXITSTATUS(status) == 0 || useStdErr))
        {
            // read stdout
            debug("reading stdout");
            char c;
            while (read(pipeout.at(0), &c, 1) == 1)
                output += c;

            close(pipeout.at(0));
            if (!output.empty() && output.back() == '\n')
                output.pop_back();

            return true;
        }
        else
        {
            if (!noerror_print)
                error("Failed to execute the command: {}", fmt::join(cmd, " "));
        }
    }
    else if (pid == 0)
    {
        int nullFile = open("/dev/null", O_WRONLY | O_CLOEXEC);
        dup2(pipeout.at(1), useStdErr ? STDERR_FILENO : STDOUT_FILENO);
        dup2(nullFile, useStdErr ? STDOUT_FILENO : STDERR_FILENO);

        setenv("LANG", "C", 1);
        cmd.push_back(nullptr);
        execvp(cmd.at(0), const_cast<char* const*>(cmd.data()));

        die("An error has occurred with execvp: {}", strerror(errno));
    }
    else
    {
        close(pipeout.at(0));
        close(pipeout.at(1));
        die("fork() failed: {}", strerror(errno));
    }

    close(pipeout.at(0));
    close(pipeout.at(1));

    return false;
}

/** Executes commands with execvp() and keep the program running without existing
 * @param cmd_str The command to execute
 * @param exitOnFailure Whether to call exit(1) on command failure.
 * @return true if the command successed, else false
 */
bool taur_exec(const std::vector<std::string_view> cmd_str, const bool noerror_print)
{
    std::vector<const char*> cmd;
    for (const std::string_view str : cmd_str)
        cmd.push_back(str.data());

    int pid = fork();

    if (pid < 0)
    {
        die("fork() failed: {}", strerror(errno));
    }

    if (pid == 0)
    {
        debug("running {}", cmd);
        cmd.push_back(nullptr);
        execvp(cmd.at(0), const_cast<char* const*>(cmd.data()));

        // execvp() returns instead of exiting when failed
        die("An error has occurred: {}: {}", cmd.at(0), strerror(errno));
    }
    else if (pid > 0)
    {  // we wait for the command to finish then start executing the rest
        int status;
        waitpid(pid, &status, 0);  // Wait for the child to finish

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            return true;
        else
        {
            if (!noerror_print)
                error("Failed to execute the command: {}", fmt::join(cmd, " "));
        }
    }

    return false;
}
std::string str_tolower(std::string str)
{
    for (char& x : str)
        x = std::tolower(x);

    return str;
}

std::string str_toupper(std::string str)
{
    for (char& x : str)
        x = std::toupper(x);

    return str;
}

// http://stackoverflow.com/questions/478898/ddg#478960
std::string read_shell_exec(const std::string_view cmd)
{
    std::array<char, 1024> buffer;
    std::string            result;
    std::unique_ptr<FILE, void(*)(FILE*)> pipe(popen(cmd.data(), "r"),
    [](FILE *f) -> void
    {
        // wrapper to ignore the return value from pclose().
        // Is needed with newer versions of gnu g++
        std::ignore = pclose(f);
    });

    if (!pipe)
        die("popen() failed: {}", std::strerror(errno));

    result.reserve(buffer.size());
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();

    // why there is a '\n' at the end??
    if (!result.empty() && result.back() == '\n')
        result.pop_back();

    return result;
}

std::string getHomeCacheDir()
{
    const char* dir = std::getenv("XDG_CACHE_HOME");
    if (dir != NULL && dir[0] != '\0' && std::filesystem::exists(dir))
    {
        std::string str_dir(dir);
        if (str_dir.back() == '/')
            str_dir.pop_back();
        return str_dir;
    }
    else
    {
        const char* home = std::getenv("HOME");
        if (home == nullptr)
            die("Failed to find $HOME, set it to your home directory!");

        return std::string(home) + "/.cache";
    }
}

std::string getHomeConfigDir()
{
    const char* dir = std::getenv("XDG_CONFIG_HOME");
    if (dir != NULL && dir[0] != '\0' && std::filesystem::exists(dir))
    {
        std::string str_dir(dir);
        if (str_dir.back() == '/')
            str_dir.pop_back();
        return str_dir;
    }
    else
    {
        const char* home = std::getenv("HOME");
        if (home == nullptr)
            die("Failed to find $HOME, set it to your home directory!");

        return std::string(home) + "/.config";
    }
}

/*
 * Get the wrapup config directory
 * where we'll have "config.toml"
 * from getHomeConfigDir()
 * @return wrapup's config directory
 */
std::string getConfigDir()
{ return getHomeConfigDir() + "/wrapup"; }

/*
 * Get the tldr cache directory
 * @return tldr cache directory
 */
std::string getCacheDir()
{ return getHomeCacheDir() + "/tldr"; }
