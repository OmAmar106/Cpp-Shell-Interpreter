#include "history.h"

#include <cstdlib>
#include <fstream>

using namespace std;

static vector<string> history;
static int history_start = 0;

vector<string> get_history() {
    return history;
}

void add_history_entry(string line) {
    history.push_back(line);
}

pair<string, int> builtin_history(const vector<string>& args) {
    if (args.size() > 1 && args[1] == "-r") {
        ifstream file(args[2]);
        string line;
        while (getline(file, line)) {
            history.push_back(line);
        }
        return {"", 2};
    } else if (args.size() > 1 && args[1] == "-w") {
        ofstream file(args[2]);
        for (auto it : history) {
            file << it << endl;
        }
        return {"", 2};
    } else if (args.size() > 1 && args[1] == "-a") {
        ofstream file(args[2], ios::app);
        for (int i = history_start; i < (int)history.size(); i++) {
            file << history[i] << endl;
        }
        history_start = history.size();
        return {"", 2};
    }

    int k = history.size();
    if (args.size() > 1) {
        k = atoll(args[1].c_str());
    }
    string s;
    for (int i = history.size() - k; i < history.size(); i++) {
        s += to_string(i + 1) + " " + history[i] + (i != history.size() - 1 ? "\n" : "");
    }
    return {s, 2};
}
