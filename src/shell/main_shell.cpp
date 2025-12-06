#include <algorithm>
//#include <filesystem>
#include <iomanip>

#include "../header.h"
#include "../plugins/plugins_manager.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif


// 字符串分割辅助函数
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

void execute(const std::string& input) {
    const std::vector<std::string> cmd = split(input, ' ');
    // 修复：删除无用的空语句，并增加合理的判断逻辑
    if (cmd.empty()) {
        return;
    }

    if (cmd[0] == "cls" || cmd[0] == "clear") {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    if (cmd[0] == "cd") {
        if (cmd.size() > 3) {
            printf(RED << BOLD << "Too many arguments." << RESET);
            return;
        }

        if (cmd.size() == 1) {
            // cd without arguments
#ifdef _WIN32
            dir_now = getenv("USERPROFILE"); // Windows root
#else
            dir_now = "~"; // Unix root
#endif
            return;
        }

        std::string new_path;
        std::string path_separator;
#ifdef _WIN32
        path_separator = "\\";
        // Check if it's an absolute path (e.g., "C:\", "D:\")
        if (cmd[1].length() >= 2 && cmd[1][1] == ':') {
            new_path = cmd[1];
        }
#else
        path_separator = "/";
        if (cmd[1][0] == '/') {
            // Absolute path in Unix
            new_path = cmd[1];
        }
#endif
        else {
            // Relative path
            if (cmd[1] == "..") {
                size_t last_sep = dir_now.find_last_of("/\\");
                if (last_sep != std::string::npos && last_sep > 0) {
                    new_path = dir_now.substr(0, last_sep);
                    // Keep drive letter for Windows
#ifdef _WIN32
                    if (new_path.length() == 2 && new_path[1] == ':') {
                        new_path += path_separator;
                    }
#endif
                } else {
#ifdef _WIN32
                    new_path = "C:";
#else
                    new_path = "/";
#endif
                }
            }
            else if (cmd[1] == ".") {
                new_path = dir_now;
            }
            else {
                // Handle path concatenation
                if (dir_now.back() == '/' || dir_now.back() == '\\') {
                    new_path = dir_now + cmd[1];
                }
                else {
                    new_path = dir_now + path_separator + cmd[1];
                }
            }
        }

        // Normalize path separators for the current platform
        std::replace(new_path.begin(), new_path.end(), '/', path_separator[0]);
        std::replace(new_path.begin(), new_path.end(), '\\', path_separator[0]);

            if (is_directory_exists(new_path)) {
                dir_now = new_path;
            }
        else {
                printf(RED << BOLD << "Directory does not exist." << RESET);
            }
        }

    if (cmd[0] == "ls" || cmd[0] == "dir" || cmd[0] == "ListFiles") {
#ifdef _WIN32
        try {
            std::string searchPath = dir_now + "\\*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::string name(findData.cFileName);
                    std::string type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>   " : "<FILE>  ";
                    std::cout << std::left << std::setw(6) << type
                             << " " << name << std::endl;
                }
                while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        }
        catch (...) {
            std::cerr << RED << BOLD << "Error accessing directory." << RESET << std::endl;
        }
#else
        // Linux/Unix 实现
        DIR* dir = opendir(dir_now.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name(entry->d_name);
                if (name != "." && name != "..") {
                    struct stat statbuf;
                    std::string fullPath = dir_now + "/" + name;
                    if (stat(fullPath.c_str(), &statbuf) == 0) {
                        std::string type = S_ISDIR(statbuf.st_mode) ? "<DIR>   " : "<FILE>  ";
                        std::cout << std::left << std::setw(6) << type
                                 << " " << name << std::endl;
                    }
                }
            }
            closedir(dir);
        }
#endif
    }
    // 在 execute 函数中添加插件管理命令处理
    if (cmd[0] == "plugin" || cmd[0] == "plugins") {
        if (cmd.size() < 2) {
            printf(RED << BOLD << "Usage: plugin [install|uninstall|list|available] <plugin_name>" << RESET);
            return;
        }

        if (cmd[1] == "install" && cmd.size() == 3) {
            PluginManager::installPlugin(cmd[2]);
        }
        else if (cmd[1] == "uninstall" && cmd.size() == 3) {
            PluginManager::uninstallPlugin(cmd[2]);
        }
        else if (cmd[1] == "list") {
            PluginManager::listInstalledPlugins();
        }
        else if (cmd[1] == "available") {
            PluginManager::listAvailablePlugins();
        }
        else {
            printf(RED << BOLD << "Invalid plugin command or missing argument." << RESET);
        }
    }
}


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
            print("DUCKSHELL " << dir_now << " > ");
            std::cout.flush();
            std::getline(std::cin, command);

            if (command == "\x0C") {
                // Ctrl+L (^L)
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
                continue;
            }

            if (command == "exit" || command == "quit") {
                break;
            }

            if (!command.empty()) {
                // TODO: 处理命令执行逻辑
                printf(BOLD << YELLOW << "Executing: " << command << RESET);
                execute(command);
            }
        }
    }
    else {
        execute(param);
        startup();
    }
    return 0;
}
