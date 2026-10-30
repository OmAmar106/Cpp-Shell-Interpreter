#include "parser.h"

#include "variables.h"
#include <cctype>

using namespace std;

vector<string> parse(const string& input) {
    vector<string> args;
    string cur;
    char quote = 0;
    bool active = false;

    for (int i = 0; i < (int)input.size(); i++) {
        char c = input[i];
        if (c == '\\' && quote != '\'') {
            if (i + 1 < (int)input.size()) {
                cur += input[++i];
                active = true;
            } else {
                cur += c;
                active = true;
            }
        } else if (quote) {
            if (c == quote) {
                quote = 0;
            } else {
                cur += c;
            }
        } else if (c == '"' || c == '\'') {
            quote = c;
            active = true;
        } else if (isspace(c)) {
            if (active) {
                args.push_back(dollar(cur));
                if (args.back().empty()) {
                    args.pop_back();
                }
                cur.clear();
                active = false;
            }
        } else {
            cur += c;
            active = true;
        }
    }

    if (active) {
        args.push_back(dollar(cur));
        if (args.back().empty()) {
            args.pop_back();
        }
    }

    return args;
}

vector<string> split_pipeline(const string& input) {
    vector<string> commands;
    string cur;
    char quote = 0;

    for (int i = 0; i < (int)input.size(); i++) {
        char c = input[i];

        if (c == '\\' && quote != '\'') {
            cur += c;
            if (i + 1 < (int)input.size()) {
                cur += input[++i];
            }
        } else if (quote) {
            cur += c;
            if (c == quote) quote = 0;
        } else if (c == '"' || c == '\'') {
            quote = c;
            cur += c;
        } else if (c == '|') {
            size_t l = cur.find_first_not_of(" \t");
            size_t r = cur.find_last_not_of(" \t");

            if (l == string::npos) return {};

            commands.push_back(cur.substr(l, r - l + 1));
            cur.clear();
        } else {
            cur += c;
        }
    }

    size_t l = cur.find_first_not_of(" \t");
    size_t r = cur.find_last_not_of(" \t");

    if (!commands.empty()) {
        if (l == string::npos) return {};
        commands.push_back(cur.substr(l, r - l + 1));
    }

    return commands;
}
