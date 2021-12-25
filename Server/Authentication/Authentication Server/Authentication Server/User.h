#pragma once

using namespace std;

struct user
{
	string name;
	string password;
	vector<string> actions;
};

map<string, user> CreateUserMap();
bool login(const map<string, user>& users, const string& name, const string& pw);
