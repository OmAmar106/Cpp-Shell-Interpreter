#pragma once
#include<bits/stdc++.h>

using namespace std;

pair<string, int> exe(string input, string output_file="",bool stderr_redirect=false,bool append=false);
pair<bool,string> check_complete(string prefix);
string check_jobs();
void push_back(string line);
vector<string> get_history();
