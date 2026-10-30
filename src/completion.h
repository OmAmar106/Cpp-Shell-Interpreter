#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

std::pair<bool, std::string> check_complete(const std::string& prefix);
std::optional<std::pair<std::string, int>> builtin_complete(const std::vector<std::string>& args);
