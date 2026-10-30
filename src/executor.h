#pragma once

#include <string>
#include <utility>

using namespace std;

pair<string, int> exe(
    const string& input,
    const string& output_file = "",
    bool stderr_redirect = false,
    bool append = false
);
