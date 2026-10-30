#include "variables.h"

#include <map>

static std::map<std::string, std::string> declare_vars;

std::string dollar(const std::string& input) {
    if (input[0] == '$' && (input.size() == 1 || input[1] != '{')) {
        return declare_vars[input.substr(1, input.size() - 1)];
    }

    std::string st;
    bool f = false;
    std::string st2;
    for (int i = 0; i < (int)input.size();) {
        if (input[i] == '$' && i + 1 < (int)input.size() && input[i + 1] == '{') {
            f = true;
            i += 2;
            st2 = "";
        } else if (input[i] == '$') {
            st += declare_vars[input.substr(i + 1, input.size() - (i + 1))];
            break;
        } else {
            if (input[i] == '}') {
                i += 1;
                st += declare_vars[st2];
                f = false;
                continue;
            }
            if (f) {
                st2 += input[i];
            } else {
                st += input[i];
            }
            i += 1;
        }
    }

    return st;
}

std::pair<std::string, int> builtin_declare(const std::vector<std::string>& args) {
    if (args[1] == "-p") {
        if (declare_vars.find(args[2]) != declare_vars.end()) {
            return {"declare -- " + args[2] + "=" + "\"" + declare_vars[args[2]] + "\"", 2};
        }
        return {"declare: " + args[2] + ": not found", 2};
    } else {
        if ('0' <= args[1][0] && args[1][0] <= '9') {
            return {"declare: `" + args[1] + "': not a valid identifier", 2};
        }
        std::string lhs, rhs;
        int ind = 0;
        while (ind < (int)args[1].size() && args[1][ind] != '=') {
            lhs += args[1][ind];
            ind += 1;
        }
        ind += 1;
        while (ind < (int)args[1].size() && args[1][ind] != '=') {
            rhs += args[1][ind];
            ind += 1;
        }
        for (auto it : lhs) {
            if (!(it == '_' || ('a' <= it && it <= 'z') || ('A' <= it && it <= 'Z') || ('0' <= it && it <= '9'))) {
                return {"declare: `" + args[1] + "': not a valid identifier", 2};
            }
        }
        declare_vars[lhs] = rhs;
        return {"", 2};
    }
}
