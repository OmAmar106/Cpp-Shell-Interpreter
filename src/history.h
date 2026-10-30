#pragma once

#include <string>
#include <utility>
#include <vector>

using namespace std;

vector<string> get_history();
void add_history_entry(string line);
pair<string, int> builtin_history(const vector<string>& args);
