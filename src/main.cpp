#include "executor.h"

#include "history.h"
#include "jobs.h"
#include "repl.h"
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

using namespace std;

int main() {
    cout << unitbuf;
    cerr << unitbuf;

    const char* histfile = getenv("HISTFILE");
    string filename = histfile ? histfile : "";
    if (!filename.empty()) {
        ifstream in(filename);
        string line;
        while (getline(in, line)) {
            add_history_entry(line);
        }
    }

    while (true) {
        string completed = check_jobs();

        if (!completed.empty()) {
            cout << completed << endl;
        }

        cout << "$ ";

        string a = read_input();
        add_history_entry(a);

        if (a.empty() && cin.eof()) {
            return 0;
        }

        remember_command(a);

        try {
            pair<string, int> response = exe(a);

            if (response.second == 100) {
                if (!filename.empty()) {
                    ofstream out(filename);
                    for (auto it : get_history()) {
                        out << it << endl;
                    }
                }
                return 0;
            }

            if (!response.first.empty()) {
                cout << response.first << endl;
            }

            string s1 = check_jobs();

            if (!s1.empty()) {
                cout << s1 << endl;
            }
        } catch (exception&) {
            cout << a << ": command not found" << endl;
        }
    }
}
