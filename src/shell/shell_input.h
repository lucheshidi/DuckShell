#ifndef SHELL_INPUT_H
#define SHELL_INPUT_H

#include <string>
#include <deque>

extern std::deque<std::string> command_history;
extern size_t history_index;

std::string read_line_interactive(const std::string& prompt_shown);

#endif // SHELL_INPUT_H