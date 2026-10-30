#pragma once

#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

std::string check_jobs();
int add_job(pid_t pid, const std::string& command);
std::pair<std::string, int> builtin_jobs(const std::vector<std::string>& args);
