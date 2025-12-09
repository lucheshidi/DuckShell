#include <algorithm>
//#include <filesystem>
#include <iomanip>
#include <cctype>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#endif

#include "../header.h"
#include "../plugins/plugins_manager.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
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

// 交互式逐字符读取一行，支持在未回车时按下 Ctrl+L 清屏
static std::string read_line_interactive(const std::string& prompt_shown) {
    std::string buffer;

#ifdef _WIN32
    // Windows: 使用 _getch 逐字符读取
    for (;;) {
        int ch = _getch();

        // 回车 (CR)
        if (ch == '\r') {
            std::cout << "\n";
            break;
        }

        // 处理退格键
        if (ch == 8 /* BS */ || ch == 127) {
            if (!buffer.empty()) {
                buffer.pop_back();
                // 在控制台删除一个字符
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }

        // 处理 Ctrl+L (FF, 0x0C)
        if (ch == 12) {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            // 重绘提示符与当前缓冲
            print(prompt_shown);
            std::cout << buffer;
            std::cout.flush();
            continue;
        }

        // 忽略不可打印控制字符
        if (ch >= 32 && ch != 127) {
            buffer.push_back(static_cast<char>(ch));
            std::cout << static_cast<char>(ch);
            std::cout.flush();
        }
    }
#else
    // POSIX: 设置终端为非规范模式，逐字符读取
    termios oldt{};
    termios newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON); // 保留 ECHO 由我们自己输出
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    for (;;) {
        unsigned char ch = 0;
        ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            continue;
        }

        if (ch == '\n' || ch == '\r') {
            std::cout << "\n";
            break;
        }

        if (ch == 8 || ch == 127) { // backspace/delete
            if (!buffer.empty()) {
                buffer.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
            continue;
        }

        if (ch == 12) { // Ctrl+L
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            print(prompt_shown);
            std::cout << buffer;
            std::cout.flush();
            continue;
        }

        if (ch >= 32 && ch != 127) {
            buffer.push_back(static_cast<char>(ch));
            std::cout << static_cast<char>(ch);
            std::cout.flush();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return buffer;
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

        // Canonicalize path to remove "." and ".." segments and produce a full path style
        auto normalize_path = [&](const std::string &p) -> std::string {
            if (p.empty()) return p;

            const char sep = path_separator[0];
            std::vector<std::string> segments;

            // Detect Windows drive like "C:" and whether rooted (e.g., C:\)
            std::string drivePrefix;
            bool rooted = false;

#ifdef _WIN32
            if (p.size() >= 2 && p[1] == ':') {
                drivePrefix = p.substr(0, 2); // e.g., "C:"
                // Check if after drive there is a separator -> rooted at drive
                if (p.size() >= 3 && (p[2] == '\\' || p[2] == '/')) {
                    rooted = true;
                }
            } else if (!p.empty() && (p[0] == '\\' || p[0] == '/')) {
                rooted = true;
            }
#else
            if (!p.empty() && p[0] == '/') {
                rooted = true;
            }
#endif

            // Split by both separators
            std::string token;
            auto flush_token = [&]() {
                if (!token.empty()) {
                    if (token == ".") {
                        // skip
                    } else if (token == "..") {
                        if (!segments.empty()) {
                            segments.pop_back();
                        } else {
                            // At root: keep as root, ignore further ".."
                        }
                    } else {
                        segments.emplace_back(token);
                    }
                    token.clear();
                }
            };

            for (size_t i = 0; i < p.size(); ++i) {
                char c = p[i];
                if (c == '/' || c == '\\') {
                    flush_token();
                } else {
                    // Skip drive letters already captured
#ifdef _WIN32
                    if (i < 2 && p.size() >= 2 && p[1] == ':') {
                        // part of drive, skip storing
                        continue;
                    }
                    if (i == 2 && p.size() >= 3 && (p[2] == '\\' || p[2] == '/')) {
                        // skip the separator after drive when rooted
                        continue;
                    }
#endif
                    token.push_back(c);
                }
            }
            flush_token();

            // Rebuild
            std::string result;
#ifdef _WIN32
            result += drivePrefix;
#endif
            if (rooted) result.push_back(sep);
            for (size_t i = 0; i < segments.size(); ++i) {
                if (!(result.empty() || result.back() == sep)) {
                    result.push_back(sep);
                }
                result += segments[i];
            }

#ifdef _WIN32
            // Ensure root like "C:" becomes "C:\" to match previous behavior
            if (!drivePrefix.empty() && rooted && segments.empty()) {
                if (result.size() == 2) result.push_back(sep);
            }
#endif
            return result;
        };

        new_path = normalize_path(new_path);

            if (is_directory_exists(new_path)) {
#ifdef _WIN32
                // On Windows, correct the case of each path component and uppercase drive letter
                // using WinAPI. This ensures paths like "c:\windows" or "C:\WINDOWS" display as
                // "C:\Windows" consistently.
                #ifndef MAX_PATH
                #define MAX_PATH 260
                #endif
                char fullBuf[MAX_PATH];
                DWORD flen = GetFullPathNameA(new_path.c_str(), MAX_PATH, fullBuf, nullptr);
                std::string full = (flen > 0 && flen < MAX_PATH) ? std::string(fullBuf, flen) : new_path;

                char longBuf[MAX_PATH];
                DWORD llen = GetLongPathNameA(full.c_str(), longBuf, MAX_PATH);
                std::string cased = (llen > 0 && llen < MAX_PATH) ? std::string(longBuf, llen) : full;

                if (!cased.empty() && std::isalpha(static_cast<unsigned char>(cased[0]))) {
                    cased[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(cased[0])));
                }
                dir_now = cased;
#else
                dir_now = new_path;
#endif
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
            const std::string prompt = std::string("DUCKSHELL { ") + dir_now + " }> ";
            print(prompt);
            std::cout.flush();
            command = read_line_interactive(prompt);

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
