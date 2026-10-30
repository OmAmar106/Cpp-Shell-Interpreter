#pragma once

#include <string>
#include <utility>
#include <vector>

using namespace std;

bool is_builtin(const string& name);
pair<string, int> execute_builtin(const vector<string>& args);
