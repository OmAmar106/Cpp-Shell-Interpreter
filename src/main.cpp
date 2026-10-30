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

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    const char* histfile = getenv("HISTFILE");
    std::string filename = histfile ? histfile : "";
    if (!filename.empty()) {
        std::ifstream in(filename);
        std::string line;
        while (getline(in, line)) {
            add_history_entry(line);
        }
    }

    while (true) {
        std::string completed = check_jobs();

        if (!completed.empty()) {
            std::cout << completed << std::endl;
        }

        std::cout << "$ ";

        std::string a = read_input();
        add_history_entry(a);

        if (a.empty() && std::cin.eof()) {
            return 0;
        }

        remember_command(a);

        try {
            std::pair<std::string, int> response = exe(a);

            if (response.second == 100) {
                if (!filename.empty()) {
                    std::ofstream out(filename);
                    for (auto it : get_history()) {
                        out << it << std::endl;
                    }
                }
                return 0;
            }

            if (!response.first.empty()) {
                std::cout << response.first << std::endl;
            }

            std::string s1 = check_jobs();

            if (!s1.empty()) {
                std::cout << s1 << std::endl;
            }
        } catch (std::exception&) {
            std::cout << a << ": command not found" << std::endl;
        }
    }
}
