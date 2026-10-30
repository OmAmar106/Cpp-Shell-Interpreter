#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std;

pair<bool, string> check_complete(const string& prefix);
optional<pair<string, int>> builtin_complete(const vector<string>& args);
