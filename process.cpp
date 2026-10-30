#include "process.h"

#include "builtins.h"
#include "executor.h"
#include "jobs.h"
#include "parser.h"
#include <array>
#include <cctype>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

std::string find_executable(const std::string& name) {
    const char* path_env = getenv("PATH");
    if (path_env == nullptr) return "";

    std::stringstream paths(path_env);
    std::string dir;

    while (getline(paths, dir, ':')) {
        if (dir.empty()) dir = ".";

        std::string full_path = dir + "/" + name;
        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path;
        }
    }

    return "";
}

std::pair<std::string, int> run_external(
    std::vector<std::string>& args,
    const std::string& output_file,
    bool stderr_redirect,
    bool append
) {
    std::string execute = find_executable(args[0]);

    if (execute.empty()) {
        return {args[0] + ": not found", 1};
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (!output_file.empty()) {
            int flags = O_WRONLY | O_CREAT;
            flags |= append ? O_APPEND : O_TRUNC;
            int fd = open(output_file.c_str(), flags, 0644);
            if (fd == -1) exit(1);
            dup2(fd, stderr_redirect ? STDERR_FILENO : STDOUT_FILENO);
            close(fd);
        }

        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        execv(execute.c_str(), argv.data());
        perror("execv");
        exit(1);
    }

    if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }

    return {"", 1999};
}

std::pair<std::string, int> run_pipeline(const std::vector<std::string>& commands) {
    int n = commands.size();
    std::vector<std::array<int, 2>> pipes(n - 1);

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i].data()) == -1) {
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return {"", 1};
        }
    }

    std::vector<pid_t> pids;

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if (i < n - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < n - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            auto [output, code] = exe(commands[i]);

            if (!output.empty()) {
                write(STDOUT_FILENO, output.data(), output.size());
                write(STDOUT_FILENO, "\n", 1);
            }

            _exit(0);
        }

        if (pid < 0) {
            for (auto& p : pipes) {
                close(p[0]);
                close(p[1]);
            }

            for (pid_t child : pids) {
                waitpid(child, nullptr, 0);
            }

            return {"", 1};
        }

        pids.push_back(pid);
    }

    for (auto& p : pipes) {
        close(p[0]);
        close(p[1]);
    }

    for (pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }

    return {"", 1999};
}

std::pair<std::string, int> run_background(const std::string& input) {
    std::string command = input;
    size_t amp = command.find_last_not_of(" \t");
    if (amp != std::string::npos) {
        command.erase(amp);
    }

    while (!command.empty() && isspace(command.back())) {
        command.pop_back();
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (command.find("&&") != std::string::npos) {
            std::string part;
            size_t pos = 0;

            while ((pos = command.find("&&")) != std::string::npos) {
                part = command.substr(0, pos);
                command.erase(0, pos + 2);

                while (!part.empty() && isspace(part.back())) {
                    part.pop_back();
                }

                size_t start = part.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    part = part.substr(start);
                    auto [output, code] = exe(part);

                    if (!output.empty()) {
                        std::cout << output << '\n';
                    }

                    if (code == 100) _exit(0);
                }
            }

            size_t start = command.find_first_not_of(" \t");
            if (start != std::string::npos) {
                command = command.substr(start);
                auto [output, code] = exe(command);

                if (!output.empty()) {
                    std::cout << output << '\n';
                }
            }

            _exit(0);
        }

        std::vector<std::string> args = parse(command);
        if (args.empty()) _exit(0);

        if (is_builtin(args[0]) || args[0] == "cd") {
            auto [output, code] = exe(command);
            if (!output.empty()) std::cout << output << '\n';
            _exit(0);
        }

        std::string execute = find_executable(args[0]);
        if (execute.empty()) _exit(127);

        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        execv(execute.c_str(), argv.data());
        _exit(127);
    }

    if (pid < 0) return {"", 1};

    int next_job = add_job(pid, input);
    return {"[" + std::to_string(next_job) + "] " + std::to_string(pid), 2};
}
