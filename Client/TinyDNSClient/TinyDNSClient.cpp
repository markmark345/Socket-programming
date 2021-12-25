// TinyDNSClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <winsock.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstdio>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define MAX_BUF_LEN 4096
#define WSVERS MAKEWORLD(2, 0)

using namespace std;

void TCPdaytime(const char*, const char*);
void errexit(const char*, ...);
SOCKET connectTCP(const char*, const char*);
SOCKET connectsock(const char*, const char*, const char*);
SOCKET connectUDP(const char* host, const char* service);
void DataServer(const char* host, const char* service, const string& enc, const string& action, const string& map);
string login(const char* host, const char* service, const string& user, const string& password);
void Authorize(const char* host, const char* service, const string& enc, const string& action);

int main(int argc, char* argv[])
{
	const char* host = "localhost";
	char* service = (char*)"8080";
	WSADATA wsadata;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		errexit("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	string u = "noppharut";
	string p = "111111";
	string enc = login(host, service, u, p);
	if (!enc.empty()) {
		//Authorize(host, "8181", enc, "iptoname");
		printf("send to data server\n");
		DataServer(host, "8888", enc, "iptoname", "192.168.0.1");
	}
	WSACleanup();
	exit(0);

}

void DataServer(const char* host, const char* service, const string& enc, const string& action, const string& map) {
	char buf[MAX_BUF_LEN + 1];
	SOCKET s;
	int cc;

	memset(buf, 0, sizeof(buf));

	printf("connecting\n");
	s = connectTCP(host, service);

	string str = enc + ":" + action + ":" + map + "\n";//"eWFueWFuLjIyMjIyLmNzdHVrZXk=:iptoname:127.0.0.1\n";
	cout << str << endl;
	if (send(s, str.c_str(), str.length(), 0) == SOCKET_ERROR) {
		fprintf(stderr, "echo send error: %d \n", GetLastError());
	}

	printf("recv result...\n");
	cc = recv(s, buf, MAX_BUF_LEN, 0);
	string result = buf;
	cout << result << endl;
	//printf("%s\n", buf);

	closesocket(s);
}

void Authorize(const char* host, const char* service, const string& enc, const string& action) {
	char buf[MAX_BUF_LEN + 1];
	SOCKET s;
	int cc;

	memset(buf, 0, sizeof(buf));

	s = connectUDP(host, service);

	string up = enc + ":" + action;
	if (send(s, up.c_str(), up.length(), 0) == SOCKET_ERROR) {
		fprintf(stderr, "echo send error: %d \n", GetLastError());
	}
	cc = recv(s, buf, MAX_BUF_LEN, 0);
	string result = buf;
	cout << result << endl;
	//printf("%s\n", buf);

	closesocket(s);
}

string login(const char* host, const char* service, const string& user, const string& password) {
	char buf[MAX_BUF_LEN + 1];
	SOCKET s;
	int cc;
	int loginCount = 0;
	string result = "";

	s = connectTCP(host, service);
	//string up = "USER:gut\nPASS:333333\n";
	//string up = "USER:" + user + "\n" + "PASS:" + password + "\n";
	string up = user + "\n" + password + "\n";
	while (loginCount < 3 && result.empty()) {
		string u, p;
		if (send(s, up.c_str(), up.length(), 0) == SOCKET_ERROR) {
			fprintf(stderr, "echo send error: %d \n", GetLastError());			
		}
		cc = recv(s, buf, MAX_BUF_LEN, 0);
		result = buf;
		printf("%s\n", buf);
		loginCount++;
	}
	closesocket(s);
	return result;
}

void TCPdaytime(const char* host, const char* service) {
	char buf[MAX_BUF_LEN + 1];
	SOCKET s;
	int cc;

	s = connectTCP(host, service);

	cc = recv(s, buf, MAX_BUF_LEN, 0);

	while (cc != SOCKET_ERROR && cc > 0) {
		buf[cc] = '\0';

		(void)fputs(buf, stdout);
		cc = recv(s, buf, MAX_BUF_LEN, 0);
	}
	closesocket(s);
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

void errexit(const char* format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	WSACleanup();
	exit(1);
}


