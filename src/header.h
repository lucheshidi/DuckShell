#ifndef HEADER_H
#define HEADER_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <iterator>
#include <list>
#include <unordered_map>

#if defined(HAVE_LIBCURL)
#include <curl/curl.h>
#endif

// ANSI Color codes
const std::string RESET = "\033[0m";
const std::string BLACK = "\033[30m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";
const std::string ITALIC = "\033[3m";
const std::string UNDER = "\033[4m";
const std::string BLINK = "\033[5m";

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <direct.h>
#elif __unix__
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <pwd.h>
#endif

#define println(out) std::cout << out << std::endl;
#define print(out) std::cout << out;

// 声明全局变量（不定义）
extern std::string home_dir;
extern std::string dir_now;
extern std::unordered_map<std::string, std::string> shell_global_vars;

// 函数声明
int startup(const std::string &param = "");

// 辅助函数
inline bool is_directory_exists(const std::string& path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

#endif
