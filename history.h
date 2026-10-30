#pragma once

#include <string>
#include <utility>
#include <vector>

std::vector<std::string> get_history();
void add_history_entry(std::string line);
std::pair<std::string, int> builtin_history(const std::vector<std::string>& args);
