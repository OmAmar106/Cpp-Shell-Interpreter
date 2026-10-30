# Cpp-Shell-Interpreter

A POSIX-style shell written from scratch in C++.

Cpp-Shell-Interpreter is an interactive command-line shell that implements a practical subset of common shell behavior: command execution, builtins, quoting, variable expansion, redirection, pipelines, tab completion, background jobs, and history.

## Features

- Interactive REPL with a custom line editor
- External command execution using `PATH` lookup and `fork` / `exec`
- Single quotes, double quotes, and backslash escaping
- Variable storage and expansion with `$VAR` and `${VAR}`
- Input/output redirection with `>`, `>>`, `1>`, `1>>`, `2>`, and `2>>`
- Pipelines with `|`
- Background jobs with `&`
- Tab completion for command names and filenames
- Custom programmable completion with `complete`
- In-memory command history with file import and export
- Job tracking with running and completed job notifications
- Modular C++23 codebase with no external runtime dependencies

## Builtins

| Command | Description |
| --- | --- |
| `echo` | Print arguments to standard output |
| `exit` | Exit the shell |
| `type` | Show whether a command is a builtin or an executable |
| `pwd` | Print the current working directory |
| `cd` | Change the current working directory |
| `complete` | Register, inspect, or remove custom completion rules |
| `jobs` | List background jobs |
| `history` | Display, read, write, or append command history |
| `declare` | Create and inspect shell variables |

## Requirements

- Linux or another Unix-like operating system
- A C++23-compatible compiler such as `g++` or `clang++`

## Build

```sh
git clone https://github.com/OmAmar106/Cpp-Shell-Interpreter.git
cd Cpp-Shell-Interpreter
g++ -std=c++23 -O2 -o Cpp-Shell-Interpreter src/*.cpp
```

## Run

```sh
./Cpp-Shell-Interpreter
```

You can also pipe commands directly into the shell:

```sh
printf 'echo hello\npwd\nexit\n' | ./Cpp-Shell-Interpreter
```

## Usage

### Commands and arguments

```sh
$ echo Hello, world!
Hello, world!
$ pwd
/home/user
$ cd /tmp
$ pwd
/tmp
```

### Variables

```sh
$ declare name=world
$ echo Hello, $name!
Hello, world!
$ echo ${name}
world
```

### Redirection

```sh
$ echo first > output.txt
$ echo second >> output.txt
$ cat output.txt
first
second
```

### Pipelines

```sh
$ echo hello | tr a-z A-Z
HELLO
$ cat output.txt | grep second
second
```

### Background jobs

```sh
$ sleep 5 &
[1] 12345
$ jobs
[1]+  Running                 sleep 5 &
```

### History

```sh
$ history
$ history 10
$ history -w ~/.cpp_shell_history
$ history -a ~/.cpp_shell_history
$ history -r ~/.cpp_shell_history
```

Set `HISTFILE` to load history on startup and save it on exit:

```sh
export HISTFILE="$HOME/.cpp_shell_history"
./cpp-shell
```

### Tab completion

- Press `Tab` to complete command names.
- Press `Tab` to complete filenames and directories.
- Press `Tab` twice to list multiple matches.
- Use the up and down arrow keys to navigate command history.

### Custom completion

```sh
$ complete -C /path/to/completion-script mycommand
$ complete -p mycommand
$ complete -r mycommand
```

## Project Structure

| File | Responsibility |
| --- | --- |
| `main.cpp` | Entry point and REPL loop |
| `repl.cpp` | Line editing, history navigation, and tab completion |
| `executor.cpp` | Command dispatch, redirection, and background execution |
| `parser.cpp` | Argument parsing and pipeline splitting |
| `builtins.cpp` | Builtin command detection and execution |
| `variables.cpp` | Variable storage and expansion |
| `history.cpp` | Command history management |
| `jobs.cpp` | Background job tracking |
| `completion.cpp` | Programmable completion support |
| `process.cpp` | External commands, pipelines, and background processes |
