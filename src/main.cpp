#include "executor.h"
#include <iostream>
#include <string>
#include<bits/stdc++.h>
#include <termios.h>
#include <cstdio>
#include<filesystem>

using namespace std;
namespace fs = std::filesystem;

string read_input() {
	set<string> builtin = {"exit","echo"};

	const char* path_env = getenv("PATH");
	if(path_env!=nullptr){
		stringstream paths(path_env);
		string dir;

		while (getline(paths, dir, ':')) {
			if (dir.empty()) dir = ".";
			fs::path dir1 = dir;
			if(fs::exists(dir1) && fs::is_directory(dir1)){
				for(const auto& entry:fs::directory_iterator(dir1)){
					if (access(entry.path().c_str(), X_OK) == 0) {
						builtin.insert(entry.path().filename().string());
					}
				}
			}
			
		}
	}

	if (!isatty(STDIN_FILENO)) {
		string input;
		if (!getline(cin, input)) return "";
		return input;
	}

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    string input;
    char c;

	bool f = false;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n' || c == '\r') {
            cout << '\n';
            break;
        } else if (c == '\t') {
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

				if (matches.size() == 1) {
					string completion = matches[0].substr(name.size());
    				completion += fs::is_directory(dir+'/'+matches[0]) ? '/' : ' ';
					cout << completion;
					input += completion;
					f = false;
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
				cout << matches[0].substr(input.size())<<" ";
				input = matches[0]+" ";
				f = false;
			} else if (!matches.empty()) {
				string st = matches[0];
				for(auto it:matches){
					for(int j=0;j<it.size();j++){
						if(st[j]!=it[j]){
							st = st.substr(0,j);
							break;
						}
					}
				}
				if(st==input){
					// cout<<f<<" "<<st<<input<<endl;
					if(f){
						f = false;
						cout<<endl;
						for(auto it:matches){
							cout<<it<<" ";
						}
						cout<<endl;
						cout<<"$ "<<input;
						continue;
					}
					cout << '\a';
					f = true;
				}
				else{
					while(input!=st){
						cout<<st[input.size()];
						input += st[input.size()];
					}
					f = false;
				}
			}
			else{
				f = false;
				cout<<'\x07';
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

int main() {
	// Flush after every std::cout / std:cerr
	cout << unitbuf;
	cerr << unitbuf;

	// TODO: Uncomment the code below to pass the first stage

	while(1){
		cout << "$ ";
		
		string a = read_input();

		try{
			pair<string,int> response = exe(a);
			if(response.second==100){
				return 0;
			}
			if(response.first.empty()){
				continue;
			}
			cout<<response.first<<endl;
		}
		catch(exception &e){
			cout<<a<<": command not found"<<endl;
		}		
	}

}
