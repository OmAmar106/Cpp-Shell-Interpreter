#include "completion.h"

#include <cstdlib>
#include <map>
#include <sys/wait.h>
#include <unistd.h>

static std::map<std::string, std::string> mapping;

std::pair<bool, std::string> check_complete(const std::string& prefix) {
    if (prefix.empty()) return {false, ""};

    std::string command = prefix.substr(0, prefix.find(' '));

    size_t lastSpace = prefix.find_last_of(' ');
    std::string current = lastSpace == std::string::npos ? prefix : prefix.substr(lastSpace + 1);

    std::string previous;
    if (lastSpace != std::string::npos && lastSpace > 0) {
        size_t prevSpace = prefix.find_last_of(' ', lastSpace - 1);
        previous = prefix.substr(
            prevSpace == std::string::npos ? 0 : prevSpace + 1,
            lastSpace - (prevSpace == std::string::npos ? 0 : prevSpace + 1)
        );
    }

    if (mapping.find(command) == mapping.end()) {
        return {false, ""};
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) return {false, ""};

    pid_t pid = fork();

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        setenv("COMP_LINE", prefix.c_str(), 1);
        std::string point = std::to_string(prefix.size());
        setenv("COMP_POINT", point.c_str(), 1);

        execlp(mapping[command].c_str(), mapping[command].c_str(),
            command.c_str(), current.c_str(), previous.c_str(), nullptr);
        _exit(127);
    }

    close(pipefd[1]);

    if (pid < 0) {
        close(pipefd[0]);
        return {false, ""};
    }

    std::string output;
    char buffer[4096];
    ssize_t n;

    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, n);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return {false, ""};
    }

    return {!output.empty(), output};
}

std::optional<std::pair<std::string, int>> builtin_complete(const std::vector<std::string>& args) {
    if (args[1] == "-p") {
        if (mapping.find(args[2]) != mapping.end()) {
            return std::make_pair("complete -C '" + mapping[args[2]] + "' " + args[2], 2);
        }
        return std::make_pair("complete: " + args[2] + ": no completion specification", 4);
    } else if (args[1] == "-C") {
        mapping[args[3]] = args[2];
        return std::make_pair("", 2);
    } else if (args[1] == "-r") {
        mapping.erase(mapping.find(args[2]));
        return std::make_pair("", 2);
    }
    return std::nullopt;
}
