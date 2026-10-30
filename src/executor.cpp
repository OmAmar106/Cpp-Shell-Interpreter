#include "executor.h"

#include "builtins.h"
#include "parser.h"
#include "process.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>

using namespace std;

pair<string, int> exe(
    const string& input,
    const string& output_file,
    bool stderr_redirect,
    bool append
) {
    vector<string> list_input = parse(input);

    if (list_input.empty()) return {"", 1};

    vector<string> commands = split_pipeline(input);

    if (commands.size() > 1) {
        return run_pipeline(commands);
    }

    if (!output_file.empty()) {
        int flags = O_WRONLY | O_CREAT;
        flags |= append ? O_APPEND : O_TRUNC;
        int fd = open(output_file.c_str(), flags, 0644);
        if (fd == -1) return {"", 1};
        close(fd);
    }

    if (list_input.back() == "&") {
        return run_background(input);
    }

    string st;
    for (int i = 0; i < (int)list_input.size(); i++) {
        if (list_input[i] == ">" || list_input[i] == "1>" || list_input[i] == "2>" ||
            list_input[i] == ">>" || list_input[i] == "1>>" || list_input[i] == "2>>") {
            if (i + 1 >= (int)list_input.size()) return {"", 1};

            string f_name = list_input[i + 1];
            bool err = list_input[i] == "2>" || list_input[i] == "2>>";
            bool app = list_input[i] == ">>" || list_input[i] == "1>>" || list_input[i] == "2>>";

            auto [op, code] = exe(st, f_name, err, app);

            if (code != 1999) {
                if (err) {
                    if (!op.empty()) {
                        cout << op;
                        if (op.back() != '\n') cout << '\n';
                    }
                } else {
                    ofstream file(f_name, app ? ios::app : ios::out);
                    file << op;
                    if (!op.empty() && op.back() != '\n') file << '\n';
                }
            }
            return {"", 4};
        }
        if (!st.empty()) st += ' ';
        st += list_input[i];
    }

    if (is_builtin(list_input[0]) || list_input[0] == "cd") {
        return execute_builtin(list_input);
    }

    return run_external(list_input, output_file, stderr_redirect, append);
}
