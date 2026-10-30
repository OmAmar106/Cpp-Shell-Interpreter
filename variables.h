#pragma once

#include <string>
#include <utility>
#include <vector>

std::string dollar(const std::string& input);
std::pair<std::string, int> builtin_declare(const std::vector<std::string>& args);
