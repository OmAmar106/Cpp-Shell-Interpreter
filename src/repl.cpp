#include "repl.h"

#include "completion.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

namespace fs = filesystem;

static vector<string> command_history;

static vector<string> split_candidates(const string& output) {
    vector<string> candidates;
    stringstream ss(output);
    string candidate;

    while (getline(ss, candidate)) {
        if (!candidate.empty() && candidate.back() == '\r') {
            candidate.pop_back();
        }
        if (!candidate.empty()) {
            candidates.push_back(candidate);
        }
    }

    sort(candidates.begin(), candidates.end());
    return candidates;
}

void remember_command(const string& command) {
    if (command.empty()) return;
    if (command_history.empty() || command_history.back() != command) {
        command_history.push_back(command);
    }
}

string read_input() {
    if (!isatty(STDIN_FILENO)) {
        string input;
        if (!getline(cin, input)) return "";
        return input;
    }

    set<string> builtin = {"exit", "echo"};

    const char* path_env = getenv("PATH");
    if (path_env != nullptr) {
        stringstream paths(path_env);
        string dir;

        while (getline(paths, dir, ':')) {
            if (dir.empty()) dir = ".";
            fs::path dir1 = dir;
            if (fs::exists(dir1) && fs::is_directory(dir1)) {
                for (const auto& entry : fs::directory_iterator(dir1)) {
                    if (access(entry.path().c_str(), X_OK) == 0) {
                        builtin.insert(entry.path().filename().string());
                    }
                }
            }
        }
    }

    termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        string input;
        if (!getline(cin, input)) return "";
        return input;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
        perror("tcsetattr");
        return "";
    }

    string input;
    char c;
    bool f = false;

    int history_index = command_history.size();
    string saved_input;

    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\033') {
            char seq[2];

            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    if (history_index == (int)command_history.size()) {
                        saved_input = input;
                    }

                    if (history_index > 0) {
                        history_index--;
                        input = command_history[history_index];
                    }
                } else if (seq[1] == 'B') {
                    if (history_index < (int)command_history.size()) {
                        history_index++;

                        if (history_index == (int)command_history.size()) {
                            input = saved_input;
                        } else {
                            input = command_history[history_index];
                        }
                    }
                }

                cout << "\r\033[2K$ " << input;
                f = false;
            }

            continue;
        }

        if (c == '\n' || c == '\r') {
            cout << '\n';
            break;
        } else if (c == '\t') {
            auto [flag, op] = check_complete(input);

            if (flag) {
                vector<string> candidates = split_candidates(op);

                if (candidates.size() == 1) {
                    string lastWord = input.substr(input.find_last_of(' ') == string::npos
                                                            ? 0
                                                            : input.find_last_of(' ') + 1);
                    string completion = candidates[0];

                    if (completion.rfind(lastWord, 0) == 0) {
                        string suffix = completion.substr(lastWord.size());
                        cout << suffix << " ";
                        input += suffix + " ";
                    }
                    f = false;
                    continue;
                }

                if (!candidates.empty()) {
                    string lcp = candidates[0];

                    for (const auto& candidate : candidates) {
                        size_t i = 0;
                        while (i < lcp.size() && i < candidate.size() &&
                            lcp[i] == candidate[i]) {
                            i++;
                        }
                        lcp = lcp.substr(0, i);
                    }

                    size_t lastSpace = input.find_last_of(' ');
                    size_t start = lastSpace == string::npos ? 0 : lastSpace + 1;
                    string current = input.substr(start);

                    if (lcp.size() > current.size()) {
                        string suffix = lcp.substr(current.size());
                        cout << suffix;
                        input += suffix;
                        f = false;
                    } else if (f) {
                        f = false;
                        cout << '\n';
                        for (const auto& candidate : candidates) {
                            cout << candidate << "  ";
                        }
                        cout << "\n$ " << input;
                    } else {
                        f = true;
                        cout << '\a';
                    }

                    continue;
                }
            }

            if (input.find(' ') != string::npos) {
                size_t pos = input.find_last_of(' ');
                string prefix = input.substr(pos + 1);

                size_t slash = prefix.find_last_of('/');
                string dir = slash == string::npos ? "." : prefix.substr(0, slash + 1);
                string name = slash == string::npos ? prefix : prefix.substr(slash + 1);

                vector<string> matches;

                if (fs::exists(dir) && fs::is_directory(dir)) {
                    for (const auto& entry : fs::directory_iterator(dir)) {
                        string filename = entry.path().filename().string();
                        if (filename.rfind(name, 0) == 0) {
                            matches.push_back(filename);
                        }
                    }
                }

                sort(matches.begin(), matches.end());

                if (matches.size() == 1) {
                    string completion = matches[0].substr(name.size());
                    completion += fs::is_directory(fs::path(dir) / matches[0]) ? '/' : ' ';
                    cout << completion;
                    input += completion;
                    f = false;
                } else if (matches.size() > 1) {
                    string common = matches[0];

                    for (const auto& match : matches) {
                        size_t j = 0;
                        while (j < common.size() && j < match.size() && common[j] == match[j]) {
                            j++;
                        }
                        common = common.substr(0, j);
                    }

                    if (common.size() > name.size()) {
                        string completion = common.substr(name.size());
                        cout << completion;
                        input += completion;
                        f = false;
                    } else if (f) {
                        f = false;
                        cout << '\n';
                        for (const auto& match : matches) {
                            cout << match;
                            if (fs::is_directory(fs::path(dir) / match)) {
                                cout << '/';
                            }
                            cout << "  ";
                        }
                        cout << "\n$ " << input;
                    } else {
                        cout << '\a';
                        f = true;
                    }
                } else {
                    cout << '\a';
                    f = false;
                }

                continue;
            }

            vector<string> matches;

            for (const auto& cmd : builtin) {
                if (cmd.rfind(input, 0) == 0) {
                    matches.push_back(cmd);
                }
            }

            if (matches.size() == 1) {
                string completion = matches[0].substr(input.size()) + " ";
                cout << completion;
                input += completion;
                f = false;
            } else if (!matches.empty()) {
                string st = matches[0];
                for (const auto& it : matches) {
                    size_t j = 0;
                    while (j < st.size() && j < it.size() && st[j] == it[j]) {
                        j++;
                    }
                    st = st.substr(0, j);
                }

                if (st == input) {
                    if (f) {
                        f = false;
                        cout << '\n';
                        for (const auto& it : matches) {
                            cout << it << "  ";
                        }
                        cout << "\n$ " << input;
                    } else {
                        cout << '\a';
                        f = true;
                    }
                } else {
                    string completion = st.substr(input.size());
                    cout << completion;
                    input += completion;
                    f = false;
                }
            } else {
                f = false;
                cout << '\a';
            }
        } else if (c == 127 || c == '\b') {
            if (!input.empty()) {
                input.pop_back();
                cout << "\b \b";
                f = false;
            }
        } else {
            f = false;
            input += c;
            cout << c;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return input;
}
