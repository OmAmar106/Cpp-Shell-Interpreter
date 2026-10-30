#pragma once

#include <string>
#include <utility>
#include <vector>

std::string find_executable(const std::string& name);
std::pair<std::string, int> run_external(
    std::vector<std::string>& args,
    const std::string& output_file,
    bool stderr_redirect,
    bool append
);
std::pair<std::string, int> run_pipeline(const std::vector<std::string>& commands);
std::pair<std::string, int> run_background(const std::string& input);
