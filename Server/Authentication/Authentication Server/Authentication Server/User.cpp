#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <map>
#include <iterator>
#include <vector>

#include "User.h"

map<string, user> CreateUserMap() {
	map<string, user> users;

	ifstream file("user_pass_action.csv");
	string str;
	bool first = true;
	while (getline(file, str)) {
		if (first) {
			first = false;
			continue;
		}
		//cout << str << "\n";
		string name;
		string value;
		string action;
		int count = 0;
		char* token = strtok((char*)str.c_str(), ",");
		while (token != NULL)
		{
			if (count == 0) {
				name = token;
			}
			else if (count == 1) {
				value = token;
			}
			else {
				action = token;
			}
			count++;
			//printf("%s\n", token);
			token = strtok(NULL, ",");
		}

		vector<string> actions;
		token = strtok((char*)action.c_str(), ":");
		while (token != NULL)
		{
			actions.push_back(token);
			//printf("%s\n", token);
			token = strtok(NULL, ":");
		}

		user u = { name, value, actions };
		users.insert(pair<string, user>(name, u));
	}
	return users;
}

bool login(const map<string, user>& users, const string& name, const string& pw) {
	map<string, user>::const_iterator itr = users.find(name);
	if (itr != users.end()) {
		cout << "user name : " << itr->first << " password : " << itr->second.password << endl;
		
		if (itr->second.password != pw) {
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}