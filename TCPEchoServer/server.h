#pragma once
#include<ws2tcpip.h>
#include<winsock2.h>
#include<process.h>
#include<memory>
#include<thread>
#include<stdio.h>
#include<iostream>
#include<string>
#include<regex>
#include<vector>
#include<mutex>
#include<chrono>
#include <fstream>
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
	Server(char ip[]);
	~Server();
	bool start();
	void startThreadPorts();
	void servClient();
	void printBuffer(char* bufferPtr, int size);
	void putClientOnDiferrentPort(SOCKET clientInstance, int portNew);
	void getClientResource(SOCKET clientInstance);
	void openFileWithPathAndSend(string filePath, SOCKET clientInstance);
	void sendFile(FILE* m_file, SOCKET clientInstance);
	string processRequest(char* bufferPtr);
	string getFilePath(string p_toParse);
	void printClientPortAndIP(SOCKET clientInstance);
	int roundRobinGetNextPort();
private:
	FILE * logFile;
	int iResult;
	WSADATA wsaData;
	vector<std::thread> portThreads; //this is vector which holds all my diferent servers on diferent ports
	int port = 2000; //startPort - the starting and also main listening port which will accept all trafic and sends out the ports for connecting
	int numServerPorts = 5;//numServerPorts - number of ports which is server listening on
	SOCKET listenSock;
	SOCKET client;//create my listening socket and client socket which will be different client each time
	//static const char SERVER_IP[]; //default server ip (localhost)
	//static const string REGEX_GET;
	std::mutex g_lockPrint;
	std::mutex g_lockCounter;
	std::recursive_mutex g_r_lock;
	int g_counter{ 1 };
	int g_clientId{ 0 };
};

