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
#include "../plugins/plugins_interface.h"
#include <deque>
#include <string>

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
            std::string custom_prompt = "";
            // 尝试从已加载的插件中获取 prompt
            for (auto plugin : loaded_plugin_instances) {
                std::string p = plugin->get_prompt();
                if (!p.empty()) {
                    custom_prompt = p;
                    break; // 使用第一个提供 prompt 的插件
                }
            }

            const std::string prompt = custom_prompt.empty() 
                ? (std::string("DUCKSHELL { ") + dir_now + " }> ")
                : custom_prompt;
                
            print(prompt);
            std::cout.flush();
            command = read_line_interactive(prompt);

            // 添加命令到历史记录（除非是空命令或与上一条相同）
            if (!command.empty() && (command_history.empty() || command_history.back() != command)) {
                command_history.push_back(command);
                if (command_history.size() > 100) {
                    command_history.pop_front();
                }
            }
            history_index = command_history.size();

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