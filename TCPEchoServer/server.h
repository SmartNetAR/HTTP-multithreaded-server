#pragma once

//mainthreadsock define the entry point for console application
#define WIN32_LEAN_AND_MEAN

#include <ws2tcpip.h>
#include <winsock2.h>
#include <process.h>
#include <memory>
#include <thread>
#include <stdio.h>
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>

#include "ServerAPI.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
using std::cin;		using std::cout;
using std::endl;	using std::string;
using std::regex;   using std::regex_token_iterator;
using std::unique_ptr;
using std::make_unique;
using std::thread;
using std::vector;
using std::ofstream;

class Server
{
public:
	Server();
	~Server();
	bool start();
	void startThreadPorts(int i);
	void servClient(SOCKET client, int port);
	void printBuffer(char* bufferPtr, int size);
	void putClientOnDiferrentPort(SOCKET client, int port);
	void getClientResource(SOCKET client);
	void openFileWithPathAndSend(string filePath, SOCKET client);
	void sendFile(FILE* m_file, SOCKET client);
	string processRequest(char* bufferPtr);
	string getFilePath(string p_toParse);
	int roundRobinGetNextPort(int port);
	void printClientPortAndIP(SOCKET client, int port);
private:
	static const char SERVER_IP[]; //default server ip (localhost)
	static const string REGEX_GET;
	FILE * logFile;
	vector<std::thread> portThreads; //this is vector which holds all my diferent servers on diferent ports
	const int initialPort = 2000; //start Port - the starting and also main listening port which will accept all trafic and sends out the ports for connecting
	//int port; // actual port / listening port
	int numServerPorts = 5;//numServerPorts - number of ports which is server listening on
	std::mutex g_lockPrint;
	std::mutex g_lockCounter;
	std::recursive_mutex g_r_lock;
	int g_counter{ 1 };
	int g_clientId{ 0 };
	int clientNum = 0;
	int breakAfterServed = 100; //this is the number of clients that will be served by the server
};

