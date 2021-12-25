
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <map>
#include <iterator>

#include "DataList.h"

//using namespace std;

map<string, string> CreateDataListMap() {
	map<string, string> serverConfig;
	ifstream file("data_list.csv");
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
		bool firstToken = true;
		char* token = strtok((char*)str.c_str(), ",");
		while (token != NULL)
		{
			if (firstToken) {
				firstToken = false;
				name = token;
			}
			else {
				value = token;
			}
			//printf("%s\n", token);
			token = strtok(NULL, ",");
		}
		serverConfig.insert(pair<string, string>(name, value));
	}
	return serverConfig;
}
