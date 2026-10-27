#include<bits/stdc++.h>
#include <cstddef>
#include <unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<cstdlib>

using namespace std;

set<string> builtin = {
    "echo",
    "exit",
    "type"
};

pair<string,int> exe(string input){
	
	vector<string> list_input;
	int i = 0;

	while(i<input.size()){
		int j = i;
		while(i<input.size() && input[i]!=' '){
			i += 1;
		}
		string st = input.substr(j,i-j);
		list_input.push_back(st);
		
		i += 1;
	}

	if(list_input[0]=="exit"){
		return {"",100};
	}
	else if(list_input[0]=="echo"){
		string st2;
		for(int i=1;i<list_input.size();i++){
			st2 += list_input[i];
			if(i!=list_input.size()-1)st2 += ' ';
		}
		return {st2,1};
	}
	else if(list_input[0]=="type"){
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
	else{
		auto [resp,code] = exe("type "+input);
		if(code==3){
			int i = resp.size()-1;
			while(resp[i]!=' '){
				i -= 1;
			}
			string execute = resp.substr(i+1,resp.size()-i);
			
			pid_t pid = fork();
			if(pid==0){
				vector<char*> argv;
				for (auto& arg : list_input) {
					argv.push_back(arg.data());
				}
				argv.push_back(nullptr);
				execv(execute.c_str(),argv.data());
				perror("execv");
				exit(1);
			}
			else{
				waitpid(pid,nullptr,0);
			}
			return {"",4};
		}
	}
	throw invalid_argument("command does not exist");
}