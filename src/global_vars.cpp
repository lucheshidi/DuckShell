// global_vars.cpp
#include "header.h"

// 定义全局变量
std::string home_dir = []() {
    char* env = nullptr;
#ifdef _WIN32
    env = getenv("USERPROFILE");
#else
    env = getenv("HOME");
#endif
    return env ? std::string(env) : ".";
}();

std::string dir_now = home_dir;
std::unordered_map<std::string, std::string> shell_global_vars;
