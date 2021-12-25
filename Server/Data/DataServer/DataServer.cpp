// DataServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <cstdio>
#include <vector>
#include <map>
#include <iterator>
#include <winsock2.h>
#include <time.h>
#include <process.h>

#include "ServerConfig.h"
#include "User.h"
#include "Base64.h"
#include "DataList.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

using namespace std;

SOCKET passiveTCP(const char* service, int qlen);
SOCKET passiveUDP(const char* service, int qlen);
SOCKET passivesock(const char*, const char*, int);
SOCKET connectTCP(const char*, const char*);
SOCKET connectsock(const char*, const char*, const char*);
SOCKET connectUDP(const char* host, const char* service);
string Authorize(const char* host, const char* service, const string& enc, const string& action);
string parseAuthorizeResult(const string& result);
string getActionIpToName(const string& map);
string getActionNameToIp(const string& map);
void errexit(const char*, ...);
void TCPdaytimed(SOCKET);
void getAction(SOCKET fd);
bool checkActionIsQuit(string action);
bool parseResultAuthorize(const char* host, const char* port, string enc, string action);

#define QLEN 5
#define STKSIZE 16536
#define BUFFSIZE 4096
#define MAX_BUF_LEN 4096


u_short portbase = 0;
map<string, string> serverConfig_;
map<string, user> users_;
map<string, string> dataList_;

const char* host = "localhost";

string parseAuthorizeResult(const string& result) {
	int count = 0;
	string action;
	string strbool = "";

	char* token = strtok((char*)result.c_str(), ":");
	while (token != NULL)
	{
		if (count == 0) {
			action = token;
			//action.erase(remove_if(action.begin(), action.end(), isspace), action.end());
		}
		else if (count == 1)
		{
			strbool = token;
			//strbool.erase(remove_if(strbool.begin(), strbool.end(), isspace), strbool.end());
		}
		count++;

		token = strtok(NULL, ":");
	}
	return strbool;
}

void parseRequest(const string& req, string& enc, string& action, string& map) {
	int count = 0;

	char* token = strtok((char*)req.c_str(), ":");
	while (token != NULL)
	{
		if (count == 0) {
			enc = token;
		}
		else if (count == 1)
		{
			action = token;
			//action.erase(remove_if(action.begin(), action.end(), isspace), action.end());
		}
		else {
			/*if (token != NULL && strlen(token) > 0) {
				char* end = token + strlen(token) - 1;
				if (*end == '\n') {
					*end = '\0';
				}
			}*/
			map = token;			
			//map.erase(remove_if(map.begin(), map.end(), isspace), map.end());
		}
		count++;

		token = strtok(NULL, ":");
	}
}

int main()
{
	struct sockaddr_in fsin;
	SOCKET msock, ssock;
	int alen;
	char* service = (char*)"8080";
	WSADATA wsadata;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		errexit("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	
	dataList_ = CreateDataListMap();
	serverConfig_ = CreateServerConfigMap();

	//string str = "eWFueWFuLjIyMjIyLmNzdHVrZXk=:iptoname:127.0.0.1\n";

	string port = serverConfig_["data_server_port"];
	service = const_cast<char*>(port.c_str());

	msock = passiveTCP(service, QLEN);
	cout << "DataServer starting" << endl;
	while (1) {
		alen = sizeof(struct sockaddr);
		ssock = accept(msock, (struct sockaddr*) & fsin, &alen);
		if (ssock == INVALID_SOCKET)
			errexit("accept faild: error number %d\n", GetLastError());

		char* clientip = inet_ntoa(fsin.sin_addr);
		//printf("%s connected\n", clientip);

		if (_beginthread((void (*) (void*))getAction, STKSIZE, (void*)ssock) < 0) {
			errexit("_beginthread: %s \n", strerror(errno));
		}
		//TCPdaytimed(ssock);
		//(void)closesocket(ssock);
	};

	closesocket(msock);

	
	WSACleanup();
	return 0;
}

void getAction(SOCKET fd) {
	char buf[BUFFSIZE];
	char buf2[BUFFSIZE];
	int cc;
	string enc, action, map, str;
	
	memset(buf, 0, sizeof(buf));
	memset(buf2, 0, sizeof(buf2));
	cc = recv(fd, buf, sizeof buf, 0);
	if (cc == SOCKET_ERROR) {
		fprintf(stderr, "receive error: %d \n", GetLastError());
		closesocket(fd);
		return;
	}

	cc = recv(fd, buf2, sizeof buf2, 0);
	if (cc == SOCKET_ERROR) {
		fprintf(stderr, "receive error: %d \n", GetLastError());
		closesocket(fd);
		return;
	}

	str = buf;
	str = str + buf2;
	parseRequest(str, enc, action, map);
	//printf("%s\n%s\n%s\n", enc.c_str(), action.c_str(), map.c_str());
	if (checkActionIsQuit(action)) {
		printf("quit\n");
		closesocket(fd);
		return;
	}
	//cout << "processing...\n";
	string result;
	string s = serverConfig_["authorize_server_port"];
	char* service = const_cast<char*>(s.c_str());
	if (parseResultAuthorize(host, service, enc, action)) {
		map.erase(remove_if(map.begin(), map.end(), isspace), map.end());
		if (action == "iptoname") {
			result = getActionIpToName(map);
		}
		else if (action == "nametoip") {
			result = getActionNameToIp(map);
		}
	}
	cout << result << endl;
	if (send(fd, result.c_str(), result.length(), 0) == SOCKET_ERROR) {
		fprintf(stderr, "send error: %d \n", GetLastError());
	}

	closesocket(fd);
}

string getActionIpToName(const string& map) {
	string value = "not found";
	for (auto it = dataList_.begin(); it != dataList_.end(); ++it) {
		//cout << "name : " << it->first << endl;
		if (it->second == map) {
			value = it->first;
			//cout << "value is: " << value << endl;
			break;
		}
	}
	return value;
}

string getActionNameToIp(const string& map) {
	string value = "not found";
	cout << "Searching..." << map << endl;
	for (auto it = dataList_.begin(); it != dataList_.end(); ++it) {
		cout << "name : " << it->first << endl;
		if (it->first == map) {
			value = it->second;
			cout << "value is: " << value << endl;
			break;
		}
	}
	return value;
}

bool parseResultAuthorize(const char* host, const char* port, string enc, string action) {
	string result = Authorize(host, port, enc, action);
	//cout << "Authorize result : " << result << endl;
	if (result == "true") {
		string value;
		//cout << "action : " << action << endl;
		return true;
		
	}
	return false;
}

bool checkActionIsQuit(string action) {
	if (action == "quit") {
		return true;
	}
	return false;
}


string Authorize(const char* host, const char* service, const string& enc, const string& action) {
	char buf[MAX_BUF_LEN + 1];
	SOCKET s;
	int cc;

	memset(buf, 0, sizeof(buf));
	//printf("before connectUDP\n");
	s = connectUDP(host, service);

	string up = enc + ":" + action;
	cout << up << endl;
	if (send(s, up.c_str(), up.length(), 0) == SOCKET_ERROR) {
		fprintf(stderr, "echo send error: %d \n", GetLastError());

	}
	cc = recv(s, buf, MAX_BUF_LEN, 0);
	string result = buf;
	cout << result << endl;
	//printf("%s\n", buf);
	string ret = parseAuthorizeResult(result);
	closesocket(s);
	return ret;
}


SOCKET passiveTCP(const char* service, int qlen) {
	return passivesock(service, "tcp", qlen);
}

SOCKET passiveUDP(const char* service, int qlen) {
	return passivesock(service, "udp", qlen);
}

SOCKET connectTCP(const char* host, const char* service) {
	return connectsock(host, service, "tcp");
}

SOCKET connectUDP(const char* host, const char* service) {
	return connectsock(host, service, "udp");
}

SOCKET connectsock(const char* host, const char* service, const char* transport) {
	struct hostent* phe;
	struct servent* pse;
	struct protoent* ppe;
	struct sockaddr_in sin;
	int s, type;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	if (pse = getservbyname(service, transport))
		sin.sin_port = pse->s_port;
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);

	if (phe = gethostbyname(host))
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
		errexit("can't get \"%s\" host entry\n", host);

	if ((ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocal entry\n", transport);

	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

	s = socket(PF_INET, type, ppe->p_proto);
	if (s == INVALID_SOCKET)
		errexit("socket create socket: %d\n", GetLastError());

	if (connect(s, (struct sockaddr*) & sin, sizeof(sin)) == SOCKET_ERROR)
		errexit("can't connnect to %s . %s: %d\n", host, service, GetLastError());

	return s;
}

SOCKET passivesock(const char* service, const char* transport, int qlen) {
	struct servent* pse;
	struct protoent* ppe;
	struct sockaddr_in sin;
	SOCKET s;
	int type;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	if (pse = getservbyname(service, transport))
		sin.sin_port = htons(ntohs((u_short)pse->s_port) + portbase);
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);

	if ((ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocal entry\n", transport);

	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;

	s = socket(PF_INET, type, ppe->p_proto);
	if (s == INVALID_SOCKET)
		errexit("socket create socket: %d\n", GetLastError());

	if (bind(s, (struct sockaddr*) & sin, sizeof(sin)) == SOCKET_ERROR)
		errexit("can't bind to %s port: %d\n", service, GetLastError());

	if (type == SOCK_STREAM && listen(s, qlen) == SOCKET_ERROR)
		errexit("can't listen on %s port: %d\n", service, GetLastError());

	return s;
}

void TCPdaytimed(SOCKET fd) {
	char* pts;
	time_t now;

	(void)time(&now);
	pts = ctime(&now);
	(void)send(fd, pts, strlen(pts), 0);
}

void errexit(const char* format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	WSACleanup();
	exit(1);
}
