#include <algorithm>
#include <iomanip>
#include <cctype>
#include <deque>
#include <vector>
#include <sstream>
#include <cstdint>
#include <regex>

#include "../header.h"
#include "shell_commands.h"
#include "shell_input.h"
#include "../plugins/plugin_manager.h"

/**
 * The startup function for DuckShell includes no input parameter and yes parameters and returns an exit code.
 * @param param
 * @return Exit Code
 */
int startup(const std::string &param) {
    // 初始化插件系统
    PluginManager::loadPlugins();

    if (param.empty()) {
        std::string command;
        while (true) {
            const std::string prompt = std::string("DUCKSHELL { ") + dir_now + " }> ";
            print(prompt);
            std::cout.flush();
            command = read_line_interactive(prompt);

            if (command == "exit" || command == "quit") {
                break;
            }

            if (!command.empty()) {
                // TODO: 处理命令执行逻辑
                println(BOLD << YELLOW << "Executing: " << command << RESET);
                if (const int exit_code = execute_command(command); exit_code != 0) {
                    // TODO
                }
            }
        }
    }
    else {
        execute_command(param);
    }
    return 0;
}