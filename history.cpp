#include "history.h"

#include <cstdlib>
#include <fstream>

static std::vector<std::string> history;
static int history_start = 0;

std::vector<std::string> get_history() {
    return history;
}

void add_history_entry(std::string line) {
    history.push_back(line);
}

std::pair<std::string, int> builtin_history(const std::vector<std::string>& args) {
    if (args.size() > 1 && args[1] == "-r") {
        std::ifstream file(args[2]);
        std::string line;
        while (getline(file, line)) {
            history.push_back(line);
        }
        return {"", 2};
    } else if (args.size() > 1 && args[1] == "-w") {
        std::ofstream file(args[2]);
        for (auto it : history) {
            file << it << std::endl;
        }
        return {"", 2};
    } else if (args.size() > 1 && args[1] == "-a") {
        std::ofstream file(args[2], std::ios::app);
        for (int i = history_start; i < (int)history.size(); i++) {
            file << history[i] << std::endl;
        }
        history_start = history.size();
        return {"", 2};
    }

    int k = history.size();
    if (args.size() > 1) {
        k = atoll(args[1].c_str());
    }
    std::string s;
    for (int i = history.size() - k; i < history.size(); i++) {
        s += std::to_string(i + 1) + " " + history[i] + (i != history.size() - 1 ? "\n" : "");
    }
    return {s, 2};
}
