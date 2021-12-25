// AuthorizeServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
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

SOCKET passiveUDP(const char* service, int qlen);
SOCKET passivesock(const char*, const char*, int);
void errexit(const char*, ...);
string Validate(char* buf);
void CheckRight(SOCKET);
int iResult;

using namespace std;

#define QLEN 5
#define STKSIZE 16536
#define BUFFSIZE 4096

u_short portbase = 0;
map<string, string> serverConfig_;
map<string, user> users_;

int main()
{
	serverConfig_ = CreateServerConfigMap();
	users_ = CreateUserMap();

	struct sockaddr_in fsin;
	char* service = NULL;
	SOCKET msock, ssock;
	int alen = sizeof(fsin);
	WSADATA wsadata;
	fd_set rfds;

	memset(&rfds, 0, sizeof(rfds));

	char buf[BUFFSIZE];
	int cc;
	int loginCount = 0;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		errexit("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	string port = serverConfig_["authorize_server_port"];
	service = const_cast<char*>(port.c_str());
	msock = passiveUDP(service, QLEN);

	cout << "Authorize starting" << endl;
	while (1) {
		memset(buf, 0, sizeof(buf));
		FD_SET(msock, &rfds);
		if (select(FD_SETSIZE, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval *)0) == SOCKET_ERROR) 
		{
			errexit("select error: %d\n", GetLastError());
		}

		if (FD_ISSET(msock, &rfds)) {
			alen = sizeof(fsin);
			if (recvfrom(msock, buf, sizeof(buf), 0, (struct sockaddr*) & fsin, &alen) == SOCKET_ERROR) {
				errexit("recvfrom error: %d\n", GetLastError());
			}
			//printf("Data Buffer: %s \n", buf);
			string result = Validate(buf);
			cout << result << endl;
			sendto(msock, result.c_str(), result.length(), 0, (struct sockaddr*) & fsin, sizeof(fsin));
		}
		/*if (send(msock, result.c_str(), cc, 0) == SOCKET_ERROR) {
			fprintf(stderr, "send error: %d \n", GetLastError());
			break;
		}
		else {
			break;
		}*/
		/*alen = sizeof(struct sockaddr);
		ssock = accept(msock, (struct sockaddr*) & fsin, &alen);
		if (ssock == INVALID_SOCKET)
			errexit("accept faild: error number %d\n", GetLastError());

		if (_beginthread((void (*) (void*))CheckRight, STKSIZE, (void*)ssock) < 0) {
			errexit("_beginthread: %s \n", strerror(errno));
		}*/
		//TCPdaytimed(ssock);
		//(void)closesocket(ssock);
	};
	closesocket(msock);
	WSACleanup();
	return 0;
}

string Validate(char* buf) {
	string enc, action;
	string username, password;

	int count = 0;
	char* token = strtok(buf, ":");
	while (token != NULL)
	{
		if (count == 0)
		{
			++count;
			enc = token;
		}
		else if (count == 1)
		{
			++count;
			action = token;
		}

		token = strtok(NULL, ":");
	}
	string de64;
	macaron::Base64::Decode(enc, de64);
	count = 0;
	token = strtok(const_cast<char*>(de64.c_str()), ".");
	while (token != NULL)
	{
		if (count == 0)
		{
			++count;
			username = token;
		}
		else if (count == 1)
		{
			++count;
			password = token;
		}

		token = strtok(NULL, ".");
	}
	if (login(users_, username, password)) {
		user u = users_[username];

		for (auto it : u.actions) {
			if (it == action) {
				return string(action + ":true");
			}
		}
		return string(action + ":false");
	}
	return string(action + ":false");
}

void CheckRight(SOCKET fd) {
	char buf[BUFFSIZE];
	int cc;
	int loginCount = 0;

	cc = recv(fd, buf, sizeof buf, 0);
	while (cc != SOCKET_ERROR && cc > 0)
	{
		string result = Validate(buf);
		if (send(fd, result.c_str(), cc, 0) == SOCKET_ERROR) {
			fprintf(stderr, "send error: %d \n", GetLastError());
			break;
		}
		else {
			break;
		}
	}
}

void TCPdaytimed(SOCKET fd) {
	char* pts;
	time_t now;

	(void)time(&now);
	pts = ctime(&now);
	(void)send(fd, pts, strlen(pts), 0);
}

SOCKET passiveUDP(const char* service, int qlen) {
	return passivesock(service, "udp", qlen);
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

void errexit(const char* format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	WSACleanup();
	exit(1);
}


