// global_vars.cpp
#include "header.h"

// 定义全局变量
std::string home_dir = []() {
#ifdef _WIN64
return std::string(getenv("USERPROFILE"));
#else
return std::string(getenv("HOME"));
#endif
}();

std::string dir_now = home_dir;
