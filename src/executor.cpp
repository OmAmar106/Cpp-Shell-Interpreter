#include<bits/stdc++.h>
#include <string>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<cstdlib>
#include<fstream>
#include<fcntl.h>

using namespace std;

set<string> builtin = {
    "echo",
    "exit",
    "type",
    "pwd",
	"complete",
	"jobs",
	"history",
    "declare"
};

int current_job = 0;
int previous_job = 0;
int history_start = 0;

map<string,string> declare;

vector<int> job;
vector<int> process;
vector<string> com;
vector<int> job_history;

vector<string> history;

vector<string> get_history(){
    return history;
}

void push_back(string line){
	history.push_back(line);
}

void update_job_markers() {
    current_job = 0;
    previous_job = 0;

    for (int i = (int)job_history.size() - 1; i >= 0; i--) {
        int id = job_history[i];

        if (find(job.begin(), job.end(), id) == job.end()) {
            continue;
        }

        if (current_job == 0) {
            current_job = id;
        } else {
            previous_job = id;
            break;
        }
    }
}

void erase_job(int i) {
    process.erase(process.begin() + i);
    job.erase(job.begin() + i);
    com.erase(com.begin() + i);
    update_job_markers();
}


string dollar(string input){
    if(input[0]=='$' && (input.size()==1 || input[1]!='{')){
        return declare[input.substr(1,input.size()-1)];
    }

    string st;
    bool f = false;
    string st2;
    for(int i=0;i<input.size();){
        if(input[i]=='$' && i+1<input.size() && input[i+1]=='{'){
            f = true;
            i += 2;
            st2 = "";
        }
        else if(input[i]=='$'){
            st += declare[input.substr(i+1,input.size()-(i+1))];
            break;
        }
        else{
            if(input[i]=='}'){
                i += 1;
                st += declare[st2];
                f = false;
                continue;
            }
            if(f){
                st2 += input[i];
            }
            else{
                st += input[i];
            }
            i += 1;
        }

    }

    return st;
}

vector<string> parse(string input) {
    vector<string> args;
    string cur;
    char quote = 0;
    bool active = false;

    for (int i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '\\' && quote != '\'') {
            if (i + 1 < input.size()) {
                cur += input[++i];
                active = true;
            } else {
                cur += c;
                active = true;
            }
        } else if (quote) {
            if (c == quote) {
                quote = 0;
            } else {
                cur += c;
            }
        } else if (c == '"' || c == '\'') {
            quote = c;
            active = true;
        } else if (isspace(c)) {
            if (active) {
                args.push_back(dollar(cur));
                if(args.back().empty()){
                    args.pop_back();
                }
                cur.clear();
                active = false;
            }
        } else {
            cur += c;
            active = true;
        }
    }

    if (active) {
        args.push_back(dollar(cur));
        if(args.back().empty()){
            args.pop_back();
        }
    }

    return args;
}

vector<string> split_pipeline(string input) {
    vector<string> commands;
    string cur;
    char quote = 0;

    for (int i = 0; i < input.size(); i++) {
        char c = input[i];

        if (c == '\\' && quote != '\'') {
            cur += c;
            if (i + 1 < input.size()) {
                cur += input[++i];
            }
        }
        else if (quote) {
            cur += c;
            if (c == quote) quote = 0;
        }
        else if (c == '"' || c == '\'') {
            quote = c;
            cur += c;
        }
        else if (c == '|') {
            size_t l = cur.find_first_not_of(" \t");
            size_t r = cur.find_last_not_of(" \t");

            if (l == string::npos) return {};

            commands.push_back(cur.substr(l, r - l + 1));
            cur.clear();
        }
        else {
            cur += c;
        }
    }

    size_t l = cur.find_first_not_of(" \t");
    size_t r = cur.find_last_not_of(" \t");

    if (!commands.empty()) {
        if (l == string::npos) return {};
        commands.push_back((cur.substr(l, r - l + 1)));
    }

    return commands;
}

map<string,string> mapping;

string check_jobs() {
    string st;

    for (int i = 0; i < process.size();) {
        int status = 0;
        pid_t ret = waitpid(process[i], &status, WNOHANG);

        if (ret > 0 || (ret == -1 && errno == ECHILD)) {
            int finished_job = job[i];

            string marker = finished_job == current_job ? "+" :
                            finished_job == previous_job ? "-" : " ";

            if (!st.empty()) st += "\n";
            st += "[" + to_string(finished_job) + "]" + marker +
                  "  Done                    " +
                  com[i].substr(0, com[i].size() - 1);

            erase_job(i);
        } else {
            i++;
        }
    }

    return st;
}

pair<bool, string> check_complete(string prefix) {
    if (prefix.empty()) return {false, ""};

	string command = prefix.substr(0, prefix.find(' '));

	size_t lastSpace = prefix.find_last_of(' ');
	string current = lastSpace == string::npos ? prefix : prefix.substr(lastSpace + 1);

	string previous;
	if (lastSpace != string::npos && lastSpace > 0) {
		size_t prevSpace = prefix.find_last_of(' ', lastSpace - 1);
		previous = prefix.substr(
			prevSpace == string::npos ? 0 : prevSpace + 1,
			lastSpace - (prevSpace == string::npos ? 0 : prevSpace + 1)
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
		string point = to_string(prefix.size());
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

    string output;
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

    // while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
	// 	output.pop_back();
	// }
	return {!output.empty(), output};
}

pair<string,int> exe(string input,string output_file="",bool stderr_redirect=false,bool append=false){

    vector<string> list_input = parse(input);


    if(list_input.empty()) return {"",1};

    vector<string> commands = split_pipeline(input);

    if (commands.size() > 1) {
        int n = commands.size();
        vector<array<int, 2>> pipes(n - 1);

        for (int i = 0; i < n - 1; i++) {
            if (pipe(pipes[i].data()) == -1) {
                for (int j = 0; j < i; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return {"",1};
            }
        }

        vector<pid_t> pids;

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

                _exit(code == 100 ? 0 : 0);
            }

            if (pid < 0) {
                for (auto& p : pipes) {
                    close(p[0]);
                    close(p[1]);
                }

                for (pid_t child : pids) {
                    waitpid(child, nullptr, 0);
                }

                return {"",1};
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

        return {"",1999};
    }

    if(!output_file.empty()){
        int flags = O_WRONLY | O_CREAT;
        flags |= append ? O_APPEND : O_TRUNC;
        int fd = open(output_file.c_str(),flags,0644);
        if(fd==-1) return {"",1};
        close(fd);
    }


	if (list_input.back() == "&") {
		string command = input;
		size_t amp = command.find_last_not_of(" \t");
		if (amp != string::npos) {
			command.erase(amp);
		}

		while (!command.empty() && isspace(command.back())) {
			command.pop_back();
		}

		pid_t pid = fork();

		if (pid == 0) {
			if (command.find("&&") != string::npos) {
				stringstream ss(command);
				string part;
				size_t pos = 0;

				while ((pos = command.find("&&")) != string::npos) {
					part = command.substr(0, pos);
					command.erase(0, pos + 2);

					while (!part.empty() && isspace(part.back())) {
						part.pop_back();
					}

					size_t start = part.find_first_not_of(" \t");
					if (start != string::npos) {
						part = part.substr(start);
						auto [output, code] = exe(part);

						if (!output.empty()) {
							cout << output << '\n';
						}

						if (code == 100) _exit(0);
					}
				}

				size_t start = command.find_first_not_of(" \t");
				if (start != string::npos) {
					command = command.substr(start);
					auto [output, code] = exe(command);

					if (!output.empty()) {
						cout << output << '\n';
					}
				}

				_exit(0);
			}

			vector<string> args = parse(command);
			if (args.empty()) _exit(0);

			if (builtin.count(args[0]) || args[0] == "cd") {
				auto [output, code] = exe(command);
				if (!output.empty()) cout << output << '\n';
				_exit(code == 100 ? 0 : 0);
			}

			string execute;
			const char* path_env = getenv("PATH");
			stringstream paths(path_env ? path_env : "");
			string dir;

			while (getline(paths, dir, ':')) {
				if (dir.empty()) dir = ".";
				string full_path = dir + "/" + args[0];

				if (access(full_path.c_str(), X_OK) == 0) {
					execute = full_path;
					break;
				}
			}

			if (execute.empty()) _exit(127);

			vector<char*> argv;
			for (auto& arg : args) {
				argv.push_back(arg.data());
			}
			argv.push_back(nullptr);

			execv(execute.c_str(), argv.data());
			_exit(127);
		}

		if (pid < 0) return {"", 1};

		int next_job = 1;

		for (int id : job) {
			next_job = max(next_job, id + 1);
		}

		job.push_back(next_job);
		process.push_back(pid);
		com.push_back(input);

		job_history.push_back(next_job);

		previous_job = current_job;
		current_job = next_job;

		return {"[" + to_string(next_job) + "] " + to_string(pid), 2};
	}
    string st2;
    for(int i=0;i<list_input.size();i++){
        if(list_input[i]==">" || list_input[i]=="1>" || list_input[i]=="2>" || list_input[i]==">>" || list_input[i]=="1>>" || list_input[i]=="2>>"){
            if(i+1>=list_input.size()) return {"",1};

            string f_name = list_input[i+1];
            bool err = list_input[i]=="2>" || list_input[i]=="2>>";
            bool app = list_input[i]==">>" || list_input[i]=="1>>" || list_input[i]=="2>>";

            auto [op,code] = exe(st2,f_name,err,app);

            if(code!=1999){
                if(err){
                    if(!op.empty()){
                        cout<<op;
                        if(op.back()!='\n') cout<<'\n';
                    }
                }
                else{
                    ofstream file(f_name,app ? ios::app : ios::out);
                    file<<op;
                    if(!op.empty() && op.back()!='\n') file<<'\n';
                }
            }
            return {"",4};
        }
        if(!st2.empty()) st2 += ' ';
        st2 += list_input[i];
    }

    if(list_input[0]=="exit"){
        return {"",100};
    }
    else if(list_input[0]=="pwd"){
        char cwd[1000];
        return {getcwd(cwd,sizeof(cwd)),4};
    }
    else if(list_input[0]=="cd"){
        if(list_input.size()<2) return {"",1};

        string dir = list_input[1];
        if(dir=="~"){
            dir = getenv("HOME");
        }
        if(chdir(dir.c_str())==-1){
            return {"cd: "+dir+": No such file or directory",4};
        }
        return {"",1};
    }
    else if(list_input[0]=="echo"){
        string st2;
        for(int i=1;i<list_input.size();i++){
            st2 += list_input[i];
            if(i!=list_input.size()-1) st2 += ' ';
        }
        return {st2,1};
    }
    else if(list_input[0]=="type"){
        if(list_input.size()<2) return {"",1};

        string st1 = list_input[1];

        if(builtin.find(st1)!=builtin.end()){
            return {st1+" is a shell builtin",2};
        }
        const char* path_env = getenv("PATH");

        if(path_env!=nullptr){
            stringstream paths(path_env);
            string dir;
            while(getline(paths,dir,':')){
                if(dir.empty()){
                    dir = ".";
                }
                string full_path = dir+'/'+list_input[1];
                if(access(full_path.c_str(),X_OK)==0){
                    return {list_input[1]+" is "+full_path,3};
                }
            }
        }

        return {st1+": not found",1};
    }
    else if(list_input[0]=="complete"){
		if(list_input[1]=="-p"){
			if(mapping.find(list_input[2])!=mapping.end()){
				return {"complete -C '"+mapping[list_input[2]]+"' "+list_input[2],2};
			}
			return {"complete: "+list_input[2]+": no completion specification",4};
		}
		else if(list_input[1]=="-C"){
			mapping[list_input[3]] = list_input[2];
			return {"",2};
		}
		else if(list_input[1]=="-r"){
			mapping.erase(mapping.find(list_input[2]));
			return {"",2};
		}
	}
    else if (list_input[0] == "jobs") {
		string st;

		for (int i = 0; i < process.size();) {
			int status = 0;
			pid_t ret = waitpid(process[i], &status, WNOHANG);

			int id = job[i];
			string marker = id == current_job ? "+" :
							id == previous_job ? "-" : " ";

			if (!st.empty()) st += "\n";

			if (ret > 0 || (ret == -1 && errno == ECHILD)) {
				st += "[" + to_string(id) + "]" + marker +
					"  Done                    " +
					com[i].substr(0, com[i].size() - 1);

				erase_job(i);
			} else {
				st += "[" + to_string(id) + "]" + marker +
					"  Running                 " + com[i];
				i++;
			}
		}

		return {st, 2};
	}
    else if(list_input[0]=="history"){
        if(list_input.size()>1 && list_input[1]=="-r"){
            ifstream file(list_input[2]);
            string line;
            while(getline(file,line)){
                history.push_back(line);
            }
            return {"",2};
        }
        else if(list_input.size()>1 && list_input[1]=="-w"){
            ofstream file(list_input[2]);
            for(auto it:history){
                file<<it<<endl;
            }
            return {"",2};
        }
        else if(list_input.size()>1 && list_input[1]=="-a"){
            ofstream file(list_input[2], ios::app);
            for(int i=history_start;i<history.size();i++){
                file<<history[i]<<endl;
            }
            history_start = history.size();
            return {"",2};
        }
		int k = history.size();
		if(list_input.size()>1){
			k = atoll(list_input[1].c_str());
		}
        string s;
        for(int i=history.size()-k;i<history.size();i++){
            s += to_string(i+1)+" "+history[i]+(i!=history.size()-1?"\n":"");
        }
        return {s,2};
	}
    else if(list_input[0]=="declare"){
        if(list_input[1]=="-p"){
            if(declare.find(list_input[2])!=declare.end()){
                return {"declare -- "+list_input[2]+"="+"\""+declare[list_input[2]]+"\"",2};
            }
            return {"declare: "+list_input[2]+": not found",2};
        }
        else{
            if(('0'<=list_input[1][0] && list_input[1][0]<='9')){
                return {"declare: `"+list_input[1]+"': not a valid identifier",2};
            }
            string lhs,rhs;
            int ind = 0;
            while(ind<list_input[1].size() && list_input[1][ind]!='='){
                lhs += list_input[1][ind];
                ind += 1;
            }
            ind += 1;
            while(ind<list_input[1].size() && list_input[1][ind]!='='){
                rhs += list_input[1][ind];
                ind += 1;
            }
            for(auto it:lhs){
                if(!(it=='_' || ('a'<=it && it<='z') || ('A'<=it && it<='Z') || ('0'<=it && it<='9'))){return {"declare: `"+list_input[1]+"': not a valid identifier",2};}
            }
            declare[lhs] = rhs;
            return {"",2};
        }
	}
    else {
        string execute;
        const char* path_env = getenv("PATH");

        if (path_env != nullptr) {
            stringstream paths(path_env);
            string dir;

            while (getline(paths, dir, ':')) {
                if (dir.empty()) dir = ".";

                string full_path = dir + "/" + list_input[0];

                if (access(full_path.c_str(), X_OK) == 0) {
                    execute = full_path;
                    break;
                }
            }
        }

        if (execute.empty()) {
            return {list_input[0] + ": not found", 1};
        }

        pid_t pid = fork();

        if (pid == 0) {
            if (!output_file.empty()) {
                int flags = O_WRONLY | O_CREAT;
                flags |= append ? O_APPEND : O_TRUNC;
                int fd = open(output_file.c_str(),flags,0644);
                if (fd == -1) exit(1);
                dup2(fd,stderr_redirect ? STDERR_FILENO : STDOUT_FILENO);
                close(fd);
            }

            vector<char*> argv;
            for (auto& arg : list_input) {
                argv.push_back(arg.data());
            }
            argv.push_back(nullptr);

            execv(execute.c_str(),argv.data());
            perror("execv");
            exit(1);
        }

        if (pid > 0) {
            waitpid(pid,nullptr,0);
        }

        return {"",1999};
    }

    throw invalid_argument("command does not exist");
}