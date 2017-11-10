//mainthreadsock define the entry point for console application

#define WIN32_LEAN_AND_MEAN

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

static const char SERVER_IP[] = "127.0.0.1"; //default server ip (localhost)

void startThreadPorts(const int port, FILE *logFile);
void servClient(SOCKET clientInstance, int port, FILE *logfile);
void printBuffer(char* bufferPtr, int size);
void putClientOnDiferrentPort(SOCKET clientInstance,int port);
void getClientResource(SOCKET clientInstance);
void openFileWithPathAndSend(string filePath, SOCKET clientInstance);
void sendFile(FILE* m_file, SOCKET clientInstance);
string processRequest(char* bufferPtr);
string getFilePath(string p_toParse);
void printClientPortAndIP(SOCKET clientInstance, int port, FILE *logfile);
int roundRobinGetNextPort(int port);

std::mutex g_lockPrint;
std::mutex g_lockCounter;
std::recursive_mutex g_r_lock;
int g_counter{ 1 };
int g_clientId{ 0 };

//std::mutex g_mutex2;
int main()
{
	FILE * logFile;
	int err;
	err = fopen_s(&logFile, "logFile.txt", "wb");

	if (err == 0)//if i found the file i can send it back to browser
	{
		cout << "Log file : was created and opened." << endl;
	}
	else
	{
		cout << "PROBLEM" << endl;
	}
	int iResult;
	WSADATA wsaData;
	iResult=WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	vector<std::thread> portThreads; //this is vector which holds all my diferent servers on diferent ports
	int startPort = 2000; //the starting and also main listening port which will accept all trafic and sends out the ports for connecting
	int numServerPorts = 5;//number of ports which is server listening on

	for (int i = 0; i <= numServerPorts; i++)
	{	
		portThreads.push_back(std::thread(startThreadPorts, startPort + i, logFile));//create one thread per one port
	}
	for (auto& portThread : portThreads) {
		portThread.join();
	}
	fclose(logFile);
	WSACleanup();
	cin.ignore();
	return 0;
}


void startThreadPorts(const int port, FILE *logFile)
{
	//create my listening socket and client socket which will be different client each time
	SOCKET listenSock;
	SOCKET client;
	int iResult; //error handling
	//structure to hold socket data
	sockaddr_in addr;
	socklen_t sa_size = sizeof(sockaddr_in);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, SERVER_IP, &(addr.sin_addr));

	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
	}

	iResult=bind(listenSock, (sockaddr*)&addr, sizeof(sockaddr_in));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"bind function failed with error %d\n", WSAGetLastError());
		iResult = closesocket(listenSock);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		WSACleanup();
	}
	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

	cout << "Thread with id " << GetCurrentThreadId() << "is on port: " << port << endl;

	int clientNum = 0;
	int breakAfterServed = 100; //this is the number of clients that will be served by the server
	bool listening = true;
	while (listening)
	{
		//if you want to change the number of clients than you have to change the number when server stop running
		if (clientNum == breakAfterServed / 5 && port != 2000)
			{
				cout << "BREAKING " << endl;
				std::chrono::milliseconds dura(2000);
				std::this_thread::sleep_for(dura);
				break;
			}

		if (clientNum == breakAfterServed && port == 2000)
			{
				cout << "BREAKING " << endl;
				//cin.ignore();
				std::chrono::milliseconds dura(2000);
				std::this_thread::sleep_for(dura);
				break;
			}

		if ((client = accept(listenSock, (sockaddr*)&addr, &sa_size)) != INVALID_SOCKET)
		{
			/*g_lockPrint.lock();
			cout << "Received connection from " << client << endl;
			g_lockPrint.unlock();*/
			std::thread t(servClient, client, port, logFile);
			t.detach(); //this will allow the thread run on its own
			cout << "Client: " << clientNum++ << "on port " << port << endl;
			
			
		}
		else
		{
			cout << "invalid client socket" << endl;
			cin.ignore();
		}	
	}
	
}

void servClient(SOCKET clientInstance, int port, FILE *logfile)
{
	//std::lock_guard<std::recursive_mutex> lock(g_r_lock);

	if (port == 2000)
	{
		//cout << "The port is 2000 need to send them next port" << endl;
		putClientOnDiferrentPort(clientInstance,roundRobinGetNextPort(port));
	}
	else
	{
		//this is the timing part of the code
		std::chrono::time_point<std::chrono::system_clock> start, end;
		start = std::chrono::system_clock::now();
		printClientPortAndIP(clientInstance, port, logfile);

		getClientResource(clientInstance);//this function will get the webpage for client or page not found

		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::time_t end_time = std::chrono::system_clock::to_time_t(end);
		wchar_t buf[26];
		errno_t err;
		err = _wctime_s(buf, 26, &end_time);
		if (err != 0)
		{
			printf("Invalid Arguments for _wctime_s. Error Code: %d\n", err);
		}
		wprintf_s(L"The client disconected at: %s", buf);
		fwrite(buf, sizeof(wchar_t), 26, logfile);
		//myfile << (char)buf << endl;
		//cout << "The client disconected at: " << ctime_s(buf, 26, end_time) << endl;
		string requestTook = "Request took: " + std::to_string(elapsed_seconds.count()) + " seconds. \r\n";
		cout << "Request took: " << elapsed_seconds.count() << " seconds"<< endl;
		fwrite(requestTook.c_str(), sizeof(char), requestTook.size(), logfile);
		//myfile << "Request took: " << elapsed_seconds.count() << " seconds" << endl;
	}

}

//this will guaratee that each port will get diferent number
int roundRobinGetNextPort(int port)
{
	std::lock_guard<std::mutex> lock(g_lockPrint);//this allow to change the wariable only in one thread
	g_clientId++;
	port = port + g_counter;
	g_counter++;
	if (g_counter == 6)
		g_counter = 1;

	return port;
}

void printClientPortAndIP(SOCKET clientInstance, int port, FILE *logfile)
{
	//this gets clients ip from sock_addr_in
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int res = getsockname(clientInstance, (struct sockaddr *)&addr, &addr_size);
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, clientIp, INET_ADDRSTRLEN);
	string cId = "ID: " + std::to_string(g_clientId)+ " ";
	string portStr = "The Client port is: " + std::to_string(port)+ " ";
	string clientIP = "The Client IP is: ";
	clientIP.append(clientIp);
	clientIP.append(" ");
	cout << portStr << endl;
	cout << clientIP << endl;

	fwrite(cId.c_str(), sizeof(char), cId.size(), logfile);
	fwrite(portStr.c_str(), sizeof(char), portStr.size(), logfile);
	fwrite(clientIP.c_str(), sizeof(char), clientIP.size(), logfile);

}

void putClientOnDiferrentPort(SOCKET clientInstance,int port)
{
	char buffer[1024];
	int recvMsgSize;
	int iResult;
	do {

		recvMsgSize = recv(clientInstance, buffer, 1024, 0);
		if (recvMsgSize > 0)
		{
			//printf("Bytes received: %d\n", recvMsgSize);
		}
		else if (recvMsgSize == 0)
		{

			//printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}

	} while (recvMsgSize > 0);
	iResult = shutdown(clientInstance, SD_RECEIVE);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}
	string portS = std::to_string(port);

	cout << endl;
	cout << endl;
	cout << "Next client is going to reconnect on port: " << portS << endl;
	

	iResult = send(clientInstance, portS.c_str(), (int)portS.size(), 0);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}
	//shutdown the connection since no more data will be sent
	iResult = shutdown(clientInstance, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}

	// close the socket
	iResult = closesocket(clientInstance);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	//cout << "SOCKET WAS CLOSED" << endl;
}

void getClientResource(SOCKET clientInstance)
{
	string filePath = { "" };
	char buffer[1024];
	int recvMsgSize;
	int bufError;

	do {

		recvMsgSize = recv(clientInstance, buffer, 1024, 0);
		if (recvMsgSize > 0)
		{
			//printf("Bytes received: %d\n", recvMsgSize);
			filePath = processRequest(buffer);
			cout << "Extracted filename: " << filePath << endl;
			//printBuffer(buffer, recvMsgSize);

			openFileWithPathAndSend(filePath, clientInstance);
			//shutdown the connection since no more data will be sent
			bufError = shutdown(clientInstance, SD_SEND);
			if (bufError == SOCKET_ERROR) {
				wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(clientInstance);
				WSACleanup();
			}

		}
		else if (recvMsgSize == 0)
		{
			//printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}

	} while (recvMsgSize > 0);




	bufError = shutdown(clientInstance, SD_RECEIVE);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}

	//openFileWithPathAndSend(filePath, clientInstance);

	////send(clientInstance, portS.c_str(), (int)portS.size(), 0);

	//shutdown the connection since no more data will be sent
	/*bufError = shutdown(clientInstance, SD_SEND);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}*/

	// close the socket
	bufError = closesocket(clientInstance);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	//cout << "SOCKET WAS CLOSED" << endl;

}

void openFileWithPathAndSend(string filePath, SOCKET clientInstance)
{
	FILE* m_file;
	errno_t err;
	err = fopen_s(&m_file, filePath.c_str(), "r");

	if (err == 0)//if i found the file i can send it back to browser
	{
		cout << "The file :" << filePath << " was opened." << endl;
		sendFile(m_file, clientInstance);

	}
	else//i didnt find the file i have to send 404 page not found
	{
		string responseNotFound = "HTTP/1.0 404 Not Found \r\n";
		responseNotFound.append("Content-Type: text/html \r\n");
		responseNotFound.append("<HTML><HEAD><TITLE>Not Found lukas</TITLE></HEAD><BODY>Not Found</BODY></HTML>");
		cout << "Sending file not found." << endl;
		send(clientInstance, responseNotFound.c_str(), (int)responseNotFound.size(), 0);
	}

	if (m_file)//if i openet the file i have to close it 
	{
		err = fclose(m_file);
		if (err == 0)
		{
			printf("The file was closed\n");
		}
		else
		{
			printf("The file was not closed\n");
		}
	}
}

void sendFile(FILE* m_file, SOCKET clientInstance)
{
	char statusLine[] = "HTTP/1.0 200 OK\r\n";
	char contentTypeLine[] = "Content-Type: text/html\r\n";

	fseek(m_file, 0, SEEK_END);
	int bufferSize = ftell(m_file);
	//cout << "The file lenght is :" << bufferSize << endl;;
	rewind(m_file);
	//this creates unique pointer to my array 
	unique_ptr<char[]> myBufferedFile = make_unique<char[]>(bufferSize);

	//this reads whole file into buffert.
	int numRead = fread_s(myBufferedFile.get(), bufferSize, sizeof(char), bufferSize, m_file);

	int totalSend = bufferSize + strlen(statusLine) + strlen(contentTypeLine);

	unique_ptr<char[]> myUniqueBufferToSend = make_unique<char[]>(totalSend);

	memcpy(myUniqueBufferToSend.get(), &statusLine, strlen(statusLine));
	memcpy(myUniqueBufferToSend.get() + strlen(statusLine), &contentTypeLine, strlen(contentTypeLine));
	memcpy(myUniqueBufferToSend.get() + strlen(statusLine) + strlen(contentTypeLine), myBufferedFile.get(), bufferSize);

	cout << "Sending response." << endl;

	int iResult = send(clientInstance, myUniqueBufferToSend.get(), totalSend, 0);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(clientInstance);
		WSACleanup();
	}

	cout << "Total bytes send: " << iResult << endl;

}

string processRequest(char* bufferPtr)
{
	string firstLine = { "" };
	//extract the first line from buffer
	while (*bufferPtr != '\r'){
		firstLine += *bufferPtr;
		bufferPtr++;
	}
	cout << "Client request: " << firstLine << endl;
	return getFilePath(firstLine);
}

//function to get the file that client want from http request using regex
static const string REGEX_GET = R"((GET)\s\/(.+)\s(HTTP.+))";
string getFilePath(string p_toParse)
{
	regex rx(REGEX_GET);
	string extractedSubmatchPath = { "" };
	//std::cout << REGEX_GET << ": " << std::regex_match(p_toParse, rx) << '\n';

	std::smatch pieces_match;
	if (std::regex_match(p_toParse, pieces_match, rx))
	{

		std::ssub_match sub_match = pieces_match[2];
		extractedSubmatchPath = sub_match.str();
		//std::cout << "  SUBMATCH " << 2 << ": " << extractedSubmatchPath << '\n';
		//std::cout << "regexes" << '\n';
		/*for (size_t i = 0; i < pieces_match.size(); ++i)
		{
		std::ssub_match sub_match = pieces_match[i];
		std::string piece = sub_match.str();
		std::cout << "  submatch " << i << ": " << piece << '\n';
		}*/
	}
	return extractedSubmatchPath;//if there is no match so the request is not HTTP it will return empty string
}

//support function to check what is in the buffer
void printBuffer(char* bufferPtr,int size)
{
	
	for (int i = 0; i < size; i++) {
		cout << *bufferPtr;
		bufferPtr++;
	}
	
}