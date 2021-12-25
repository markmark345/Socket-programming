#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <map>
#include <iterator>

#include "ServerConfig.h"

//using namespace std;

map<string, string> CreateServerConfigMap() {
	map<string, string> serverConfig;
	ifstream file("server.config");
	string str;
	while (getline(file, str)) {
		//cout << str << "\n";
		string name;
		string value;
		bool first = true;
		char* token = strtok((char*)str.c_str(), "=");
		while (token != NULL)
		{
			if (first) {
				first = false;
				name = token;
			}
			else {
				value = token;
			}
			//printf("%s\n", token);
			token = strtok(NULL, "=");
		}
		serverConfig.insert(pair<string, string>(name, value));
	}
	return serverConfig;
}
