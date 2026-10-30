#pragma once

#include <string>
#include <utility>

std::pair<std::string, int> exe(
    const std::string& input,
    const std::string& output_file = "",
    bool stderr_redirect = false,
    bool append = false
);
