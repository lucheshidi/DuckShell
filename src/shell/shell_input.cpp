#include <algorithm>
#include <iomanip>
#include <cctype>
#include <deque>
#include <vector>
#include <sstream>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#endif

#include <cstdint>
#include <regex>

#include "../header.h"
#include "shell_input.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

// 声明全局变量用于命令历史记录
std::deque<std::string> command_history;
size_t history_index = 0;
static std::string current_buffer;
static size_t cursor_pos = 0;  // 光标在缓冲区中的位置

// 交互式逐字符读取一行，支持在未回车时按下 Ctrl+L 清屏
std::string read_line_interactive(const std::string& prompt_shown) {
    current_buffer.clear();
    cursor_pos = 0;

#ifdef _WIN32
    // Windows 实现
    for (;;) {
        int ch = _getch();

        // 特殊键处理 (功能键)
        if (ch == 0 || ch == 0xE0) {
            int ch2 = _getch(); // 获取第二个字节

            if (ch2 == 72) { // 上箭头键
                if (!command_history.empty() && history_index > 0) {
                    // 清除当前行
                    for (size_t i = 0; i < current_buffer.length(); ++i) {
                        std::cout << "\b \b";
                    }

                    // 加载历史命令
                    history_index--;
                    current_buffer = command_history[history_index];
                    cursor_pos = current_buffer.length();
                    std::cout << current_buffer;
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 80) { // 下箭头键
                if (!command_history.empty() && history_index < command_history.size()) {
                    // 清除当前行
                    for (size_t i = 0; i < current_buffer.length(); ++i) {
                        std::cout << "\b \b";
                    }

                    history_index++;
                    if (history_index < command_history.size()) {
                        current_buffer = command_history[history_index];
                    } else {
                        current_buffer.clear();
                    }
                    cursor_pos = current_buffer.length();
                    std::cout << current_buffer;
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 75) { // 左箭头键
                if (cursor_pos > 0) {
                    cursor_pos--;
                    std::cout << "\b";
                    std::cout.flush();
                }
                continue;
            }
            else if (ch2 == 77) { // 右箭头键
                if (cursor_pos < current_buffer.length()) {
                    cursor_pos++;
                    std::cout << current_buffer[cursor_pos-1];
                    std::cout.flush();
                }
                continue;
            }
        }

        // 回车 (CR)
        if (ch == '\r') {
            std::cout << "\n";
            break;
        }

        // 处理退格键
        if (ch == 8 /* BS */ || ch == 127) {
            if (!current_buffer.empty() && cursor_pos > 0) {
                current_buffer.erase(cursor_pos - 1, 1);
                cursor_pos--;

                // 重绘行：先清除整行，然后重新绘制
                std::cout << "\r" << prompt_shown << current_buffer << " "; // 添加一个空格清除最后的字符
                // 计算需要回退的距离
                size_t back_steps = current_buffer.length() - cursor_pos + 1; // +1 是为了清除添加的空格
                for (size_t i = 0; i < back_steps; ++i) {
                    std::cout << "\b";
                }
                std::cout.flush();
            }
            continue;
        }

        // 处理 Ctrl+L (FF, 0x0C)
        if (ch == 12) {
            system("cls");
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
            continue;
        }

        // 忽略不可打印控制字符
        if (ch >= 32 && ch != 127) {
            current_buffer.insert(cursor_pos, 1, static_cast<char>(ch));
            cursor_pos++;

            // 重绘行
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
        }
    }
#else
    // POSIX 版本
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

        // ANSI 转义序列处理 (方向键)
        if (ch == 27) { // ESC
            unsigned char ch2 = 0;
            n = ::read(STDIN_FILENO, &ch2, 1);
            if (n > 0 && ch2 == '[') {
                unsigned char ch3 = 0;
                n = ::read(STDIN_FILENO, &ch3, 1);
                if (n > 0) {
                    if (ch3 == 'A') { // 上箭头
                        if (!command_history.empty() && history_index > 0) {
                            // 清除当前行
                            for (size_t i = 0; i < current_buffer.length(); ++i) {
                                std::cout << "\b \b";
                            }

                            history_index--;
                            current_buffer = command_history[history_index];
                            cursor_pos = current_buffer.length();
                            std::cout << current_buffer;
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'B') { // 下箭头
                        if (!command_history.empty() && history_index < command_history.size()) {
                            // 清除当前行
                            for (size_t i = 0; i < current_buffer.length(); ++i) {
                                std::cout << "\b \b";
                            }

                            history_index++;
                            if (history_index < command_history.size()) {
                                current_buffer = command_history[history_index];
                            } else {
                                current_buffer.clear();
                            }
                            cursor_pos = current_buffer.length();
                            std::cout << current_buffer;
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'D') { // 左箭头
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            std::cout << "\b";
                            std::cout.flush();
                        }
                        continue;
                    }
                    else if (ch3 == 'C') { // 右箭头
                        if (cursor_pos < current_buffer.length()) {
                            cursor_pos++;
                            std::cout << current_buffer[cursor_pos-1];
                            std::cout.flush();
                        }
                        continue;
                    }
                }
            }
        }

        if (ch == '\n' || ch == '\r') {
            std::cout << "\n";
            break;
        }

        if (ch == 8 || ch == 127) { // backspace/delete
            if (!current_buffer.empty() && cursor_pos > 0) {
                current_buffer.erase(cursor_pos - 1, 1);
                cursor_pos--;

                // 重绘行：先清除整行，然后重新绘制
                std::cout << "\r" << prompt_shown << current_buffer << " "; // 添加一个空格清除最后的字符
                // 计算需要回退的距离
                size_t back_steps = current_buffer.length() - cursor_pos + 1; // +1 是为了清除添加的空格
                for (size_t i = 0; i < back_steps; ++i) {
                    std::cout << "\b";
                }
                std::cout.flush();
            }
            continue;
        }

        if (ch == 12) { // Ctrl+L
#ifdef _WIN32
            if (system("cls") != 0) {
                // 忽略错误
            }
#else
            if (system("clear") != 0) {
                // 忽略错误
            }
#endif
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
            continue;
        }

        if (ch >= 32 && ch != 127) {
            current_buffer.insert(cursor_pos, 1, static_cast<char>(ch));
            cursor_pos++;

            // 重绘行
            std::cout << "\r" << prompt_shown << current_buffer;
            // 将光标移动到正确位置
            for (size_t i = cursor_pos; i < current_buffer.length(); ++i) {
                std::cout << "\b";
            }
            std::cout.flush();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return current_buffer;
}