// Authentication Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
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

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

using namespace std;

SOCKET passiveTCP(const char* service, int qlen);
SOCKET passivesock(const char*, const char*, int);
void errexit(const char*, ...);
void TCPdaytimed(SOCKET);
int LoginService(SOCKET fd);
void praseUserLogin(char* buf, string& username, string& password);
int iResult;

#define QLEN 5
#define STKSIZE 16536
#define BUFFSIZE 4096

u_short portbase = 0;
map<string, string> serverConfig_;
map<string, user> users_;

string getLineValue(string& line)
{
	string name;
	string value;
	bool first = true;
	char* token = strtok((char*)line.c_str(), ":");
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
	return value;
}

int main(int argc, char* argv[])
{
	serverConfig_ = CreateServerConfigMap();
	users_ = CreateUserMap();

	struct sockaddr_in fsin;
	char* service = NULL;
	SOCKET msock, ssock;
	int alen;
	WSADATA wsadata;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		errexit("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	string port = serverConfig_["authentication_server_port"];
	service = const_cast<char*>(port.c_str());
	msock = passiveTCP(service, QLEN);
	cout << "Authorize starting" << endl;
	while (1) {
		alen = sizeof(struct sockaddr);
		ssock = accept(msock, (struct sockaddr*) & fsin, &alen);
		if (ssock == INVALID_SOCKET)
			errexit("accept faild: error number %d\n", GetLastError());

		char* clientip = inet_ntoa(fsin.sin_addr);
		//printf("%s connected\n", clientip);

		if (_beginthread((void (*) (void*))LoginService, STKSIZE, (void*)ssock) < 0) {
			errexit("_beginthread: %s \n", strerror(errno));
		}
		//TCPdaytimed(ssock);
		//(void)closesocket(ssock);
	};
	
	closesocket(msock);
	WSACleanup();
	return 0;
}

void praseUserLogin(char* buf, string& username, string& password) {
	string line1, line2;

	int count = 0;
	char* token = strtok(buf, "\n");
	while (token != NULL)
	{
		if (count == 0)
		{
			++count;
			line1 = token;
		}
		else if (count == 1)
		{
			++count;
			line2 = token;
		}

		token = strtok(NULL, "\n");
	}
	line1.erase(remove_if(line1.begin(), line1.end(), isspace), line1.end());
	line2.erase(remove_if(line2.begin(), line2.end(), isspace), line2.end());

	username = getLineValue(line1);
	password = getLineValue(line2);
}

void praseUserLoginV2(char* buf, string& username, string& password) {
	string line1, line2;

	int count = 0;
	char* token = strtok(buf, "\n");
	while (token != NULL)
	{
		if (count == 0)
		{
			++count;
			line1 = token;
		}
		else if (count == 1)
		{
			++count;
			line2 = token;
		}

		token = strtok(NULL, "\n");
	}
	line1.erase(remove_if(line1.begin(), line1.end(), isspace), line1.end());
	line2.erase(remove_if(line2.begin(), line2.end(), isspace), line2.end());
	username = line1;
	password = line2;
}

int LoginService(SOCKET fd) {
	char buf[BUFFSIZE];
	int cc;
	int loginCount = 3;
	string rcname, pw, tmp;

	while (loginCount > 0) {
		tmp = "";
		rcname = "";
		pw = "";
		memset(buf, 0, sizeof(buf));
		cc = recv(fd, buf, sizeof buf, 0);
		if (cc != SOCKET_ERROR && cc > 0) {
			tmp = buf;
			cc = recv(fd, buf, sizeof buf, 0);
			if (cc != SOCKET_ERROR && cc > 0) {
				tmp = tmp + buf;
				praseUserLogin(const_cast<char*>(tmp.c_str()), rcname, pw);
				if (login(users_, rcname, pw)) {
					user y = users_[rcname];
					string preEncode = y.name + "." + y.password + "." + serverConfig_["secret_key"];
					string en64 = macaron::Base64::Encode(preEncode);
					cout << "Before Base64 : " << preEncode << endl;
					cout << "After Base64 :  " << en64 << endl;
					//cout << "Login Ok" << endl;
					memset(buf, 0, sizeof(buf));
					memcpy(buf, en64.c_str(), en64.length());
					if (send(fd, buf, sizeof(buf), 0) == SOCKET_ERROR) {
						fprintf(stderr, "send error: %d \n", GetLastError());
					}
					goto label_exit;
				}
				else
				{
					cout << "User or Password not found!" << endl;
					if (send(fd, "not found", cc, 0) == SOCKET_ERROR) {
						fprintf(stderr, "send error: %d \n", GetLastError());
						break;
					}
					loginCount--;
				}
			}
			else {
				loginCount = 0;
			}
		}
		else {
			loginCount = 0;
		}
	}

	while (cc != SOCKET_ERROR && cc > 0)
	{
		printf("recv : buf : %s\n", buf);

		praseUserLogin(buf, rcname, pw);
		
		//praseUserLoginV2(buf, rcname, pw);
		if (login(users_, rcname, pw)) {
			user y = users_[rcname];
			string preEncode = y.name + "." + y.password + "." + serverConfig_["secret_key"];
			string en64 = macaron::Base64::Encode(preEncode);
			cout << "Before Base64 : " << preEncode << endl;
			cout << "After Base64 :  " << en64 << endl;
			//cout << "Login Ok" << endl;
			memset(buf, 0, sizeof(buf));
			memcpy(buf, en64.c_str(), en64.length());
			if (send(fd, buf, sizeof(buf), 0) == SOCKET_ERROR) {
				fprintf(stderr, "send error: %d \n", GetLastError());
				break;
			}
			goto label_exit;
		}
		else
		{
			cout << "User or Password not found!" << endl;
			if (send(fd, "not found", cc, 0) == SOCKET_ERROR) {
				fprintf(stderr, "send error: %d \n", GetLastError());
				break;
			}
			loginCount++;

			if (loginCount >= 3) {
				goto label_exit;
			}
			cc = recv(fd, buf, sizeof buf, 0);
			if (cc == SOCKET_ERROR) {
				fprintf(stderr, "recv error: %d \n", GetLastError());
				break;
			}
				
		}
	}

	label_exit: 
	closesocket(fd);
	return 0;
}

SOCKET passiveTCP(const char* service, int qlen) {
	return passivesock(service, "tcp", qlen);
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