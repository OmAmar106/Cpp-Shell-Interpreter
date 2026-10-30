#include "builtins.h"

#include "completion.h"
#include "history.h"
#include "jobs.h"
#include "process.h"
#include "variables.h"
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <unistd.h>

static const std::set<std::string> builtin = {
    "echo",
    "exit",
    "type",
    "pwd",
    "complete",
    "jobs",
    "history",
    "declare"
};

bool is_builtin(const std::string& name) {
    return builtin.find(name) != builtin.end();
}

std::pair<std::string, int> execute_builtin(const std::vector<std::string>& args) {
    if (args[0] == "exit") {
        return {"", 100};
    } else if (args[0] == "pwd") {
        char cwd[1000];
        return {getcwd(cwd, sizeof(cwd)), 4};
    } else if (args[0] == "cd") {
        if (args.size() < 2) return {"", 1};

        std::string dir = args[1];
        if (dir == "~") {
            dir = getenv("HOME");
        }
        if (chdir(dir.c_str()) == -1) {
            return {"cd: " + dir + ": No such file or directory", 4};
        }
        return {"", 1};
    } else if (args[0] == "echo") {
        std::string st;
        for (int i = 1; i < (int)args.size(); i++) {
            st += args[i];
            if (i != (int)args.size() - 1) st += ' ';
        }
        return {st, 1};
    } else if (args[0] == "type") {
        if (args.size() < 2) return {"", 1};

        std::string name = args[1];

        if (is_builtin(name)) {
            return {name + " is a shell builtin", 2};
        }

        std::string path = find_executable(name);
        if (!path.empty()) {
            return {name + " is " + path, 3};
        }

        return {name + ": not found", 1};
    } else if (args[0] == "complete") {
        auto result = builtin_complete(args);
        if (result) return *result;
        throw std::invalid_argument("command does not exist");
    } else if (args[0] == "jobs") {
        return builtin_jobs(args);
    } else if (args[0] == "history") {
        return builtin_history(args);
    } else if (args[0] == "declare") {
        return builtin_declare(args);
    }

    throw std::invalid_argument("command does not exist");
}
