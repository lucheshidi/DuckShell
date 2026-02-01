#ifndef SHELL_SYSTEM_H
#define SHELL_SYSTEM_H

#include <string>

// 包含shell系统的模块
#include "shell/shell_commands.h"
#include "shell/shell_input.h"

// 新的命名空间风格的接口，使用下划线命名法
namespace shell_system {
    inline int execute_command(const std::string& input) { return ::execute_command(input); }
    inline std::string read_line_interactive(const std::string& prompt_shown) { return ::read_line_interactive(prompt_shown); }
    inline std::vector<std::string> split_string(const std::string& str, char delimiter) { return ::split(str, delimiter); }
    inline int startup_shell(const std::string &param) { return ::startup(param); }
}

#endif // SHELL_SYSTEM_H