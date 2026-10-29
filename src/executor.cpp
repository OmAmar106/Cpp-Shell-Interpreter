#include<bits/stdc++.h>
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
    "pwd"
};

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
                args.push_back(cur);
                cur.clear();
                active = false;
            }
        } else {
            cur += c;
            active = true;
        }
    }

    if (active) args.push_back(cur);

    return args;
}

pair<string,int> exe(string input,string output_file="",bool stderr_redirect=false,bool append=false){

    vector<string> list_input = parse(input);

    if(list_input.empty()) return {"",1};

    if(!output_file.empty()){
        int flags = O_WRONLY | O_CREAT;
        flags |= append ? O_APPEND : O_TRUNC;
        int fd = open(output_file.c_str(),flags,0644);
        if(fd==-1) return {"",1};
        close(fd);
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