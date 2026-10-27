#include "executor.h"
#include <iostream>
#include <string>
#include<bits/stdc++.h>

using namespace std;


int main() {
	// Flush after every std::cout / std:cerr
	cout << unitbuf;
	cerr << unitbuf;

	// TODO: Uncomment the code below to pass the first stage

	while(1){
		cout << "$ ";
		
		string a;
		getline(cin,a);

		try{
			pair<string,int> response = exe(a);
			if(response.second==100){
				return 0;
			}
			cout<<response.first<<endl;
		}
		catch(exception &e){
			cout<<a<<": command not found"<<endl;
		}		
	}

}
