#pragma once

#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

using namespace std;

string check_jobs();
int add_job(pid_t pid, const string& command);
pair<string, int> builtin_jobs(const vector<string>& args);
