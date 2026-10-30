#pragma once

#include <string>
#include <utility>
#include <vector>

bool is_builtin(const std::string& name);
std::pair<std::string, int> execute_builtin(const std::vector<std::string>& args);
