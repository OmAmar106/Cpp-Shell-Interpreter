#pragma once

#include <string>
#include <utility>
#include <vector>

using namespace std;

string find_executable(const string& name);
pair<string, int> run_external(
    vector<string>& args,
    const string& output_file,
    bool stderr_redirect,
    bool append
);
pair<string, int> run_pipeline(const vector<string>& commands);
pair<string, int> run_background(const string& input);
