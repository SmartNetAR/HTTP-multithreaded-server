#include "server.h"

const char Server::SERVER_IP[] = "127.0.0.1"; //default server ip (localhost)
const string Server::REGEX_GET = R"((GET)\s\/(.+)\s(HTTP.+))";

Server::Server()
{
	int err;
	err = fopen_s(&logFile, "logFile.txt", "wb");

	if (err == 0)//if i found the file i can send it back to browser
	{
		cout << "Log file : was created and opened." << endl;
	}
	else
	{
		cout << "PROBLEM. Log file could not created." << endl;
	}

	int iResult;
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
	}
}

Server::~Server()
{
}


bool Server::start()
{
	
	
	for (int i = 0; i <= numServerPorts; i++)
	{
		//portThreads.push_back(std::thread(&Server::startThreadPorts, this));//create one thread per one port
		//portThreads.push_back(std::thread(&Server::startThreadPorts, this, portThread));//create one thread per one port OK?
		portThreads.push_back(std::thread(&Server::startThreadPorts, this, i));//create one thread per one port OK?
		cout << " CREATED THREAD FOR PORT " << initialPort + i << " -END-" << endl;
	}

	//std::thread t((&viewWindow::refreshWindow, render, playerRect, backTexture, playerTexture));

	//std::thread t(&viewWindow::refreshWindow, window, render, std::ref(playerRect), backTexture, playerTexture);
	//std::thread t([&](viewWindow* view) { view->refreshWindow(render, playerRect, backTexture, playerTexture); }, &window);

	for (auto& portThread : portThreads) {
		portThread.join();
	}

	fclose(logFile);
	WSACleanup();
	cin.ignore();

	return 1;
}


void Server::startThreadPorts(int i)
{
	SOCKET listenSock;
	SOCKET client;
	int port = initialPort + i;
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

	iResult = bind(listenSock, (sockaddr*)&addr, sizeof(sockaddr_in));
	if (iResult == SOCKET_ERROR) {
		wprintf(L"bind function failed with error %d\n", WSAGetLastError());
		iResult = closesocket(listenSock);
		if (iResult == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		WSACleanup();
	}
	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());

	cout << "Thread with id " << GetCurrentThreadId() << " is on port: " << port << endl << endl;

	bool listening = true;

	while (listening)
	{
		//if you want to change the number of clients than you have to change the number when server stop running
		if (clientNum == breakAfterServed / numServerPorts && port != initialPort) // port != 2000
		{
			cout << "BREAKING " << endl;
			std::chrono::milliseconds dura(2000);
			std::this_thread::sleep_for(dura);
			break;
		}

		if (clientNum == breakAfterServed && port == initialPort) // port == 2000
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
			std::thread t(&Server::servClient, this, client, port);
			//std::thread t([&](Server* s) { s->servClient(*portThread); }, this);
			t.detach(); //this will allow the thread run on its own
			cout << "Client: " << clientNum++ << "on port " << port << endl << endl;


		}
		else
		{
			cout << "invalid client socket" << endl;
			cin.ignore();
		}
	}

}

void Server::servClient(SOCKET client, int port)
{
	//std::lock_guard<std::recursive_mutex> lock(g_r_lock);

	if (port == initialPort) //2000
	{
		//cout << "The port is 2000 need to send them next port" << endl;
		Server::putClientOnDiferrentPort(client, roundRobinGetNextPort(port));
	}
	else
	{
		//this is the timing part of the code
		std::chrono::time_point<std::chrono::system_clock> start, end;
		start = std::chrono::system_clock::now();
		Server::printClientPortAndIP(client, port);

		Server::getClientResource(client);//this function will get the webpage for client or page not found

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
		fwrite(buf, sizeof(wchar_t), 26, logFile);
		//myfile << (char)buf << endl;
		//cout << "The client disconected at: " << ctime_s(buf, 26, end_time) << endl;
		string requestTook = "Request took: " + std::to_string(elapsed_seconds.count()) + " seconds. \r\n";
		cout << "Request took: " << elapsed_seconds.count() << " seconds" << endl;
		fwrite(requestTook.c_str(), sizeof(char), requestTook.size(), logFile);
		//myfile << "Request took: " << elapsed_seconds.count() << " seconds" << endl;
	}

}

//this will guaratee that each port will get diferent number
int Server::roundRobinGetNextPort(int port)
{
	std::lock_guard<std::mutex> lock(g_lockPrint);//this allow to change the wariable only in one thread
	g_clientId++;
	port = port + g_counter;
	g_counter++;
	if (g_counter == 6)
		g_counter = 1;

	return port;
}

void Server::putClientOnDiferrentPort(SOCKET client, int port)
{
	char buffer[1024];
	int recvMsgSize;
	int iResult;
	do {

		recvMsgSize = recv(client, buffer, 1024, 0);
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
	iResult = shutdown(client, SD_RECEIVE);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(client);
		WSACleanup();
	}

	string newPort = std::to_string(port);

	cout << endl;
	cout << endl;
	cout << "Next client is going to reconnect on port: " << newPort << endl;

	iResult = send(client, newPort.c_str(), (int)newPort.size(), 0);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(client);
		WSACleanup();
	}
	//shutdown the connection since no more data will be sent
	iResult = shutdown(client, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(client);
		WSACleanup();
	}

	// close the socket
	iResult = closesocket(client);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	//cout << "SOCKET WAS CLOSED" << endl;
}

void Server::getClientResource(SOCKET client)
{
	string filePath = { "" };
	char buffer[1024];
	int recvMsgSize;
	int bufError;

	do {

		recvMsgSize = recv(client, buffer, 1024, 0);
		if (recvMsgSize > 0)
		{
			//printf("Bytes received: %d\n", recvMsgSize);
			filePath = processRequest(buffer);
			cout << "Extracted filename: " << filePath << endl;
			//printBuffer(buffer, recvMsgSize);

			///testing redirections TO CONSTRUCT API APAAA
			int stringPos = filePath.find("apaaa/", 0); // Ex: APAAA/animals/1

			if (stringPos == -1)
			{
				openFileWithPathAndSend(filePath, client);
			}
			else
			{
				string request = filePath.substr(6); // Ex: animals/1
				int dashPos = request.find("/", 0); // Ex: Pos 8
				string table = request.substr(0, dashPos); // Ex: animals
				string parameter = request.substr(dashPos + 1); // Ex: 1
				cout << "request: " << request << endl;
				cout << "dashPos: " << dashPos << endl;
				cout << "table: " << table << endl;
				cout << "parameter: " << parameter << endl;
			}
			//endtesting redirections TO CONSTRUCT API APAAA

			//openFileWithPathAndSend(filePath, client); !!discomment later if you want that this works

			//shutdown the connection since no more data will be sent
			bufError = shutdown(client, SD_SEND);
			if (bufError == SOCKET_ERROR) {
				wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(client);
				WSACleanup();
			}

		}
		else if (recvMsgSize == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}

	} while (recvMsgSize > 0);

	


	bufError = shutdown(client, SD_RECEIVE);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(client);
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
	bufError = closesocket(client);
	if (bufError == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
	}

	//cout << "SOCKET WAS CLOSED" << endl;

}

void Server::openFileWithPathAndSend(string filePath, SOCKET client)
{
	FILE* m_file;
	errno_t err;
	err = fopen_s(&m_file, filePath.c_str(), "r");

	if (err == 0)//if i found the file i can send it back to browser
	{
		cout << "The file :" << filePath << " was opened." << endl;
		sendFile(m_file, client);

	}
	else//i didnt find the file i have to send 404 page not found
	{
		string responseNotFound = "HTTP/1.0 404 Not Found \r\n";
		responseNotFound.append("Content-Type: text/html \r\n");
		responseNotFound.append("<HTML><HEAD><TITLE>File Not Found</TITLE></HEAD><BODY>File Not Found</BODY></HTML>");
		cout << "Sending file not found." << endl;
		send(client, responseNotFound.c_str(), (int)responseNotFound.size(), 0);
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

void Server::sendFile(FILE* m_file, SOCKET client)
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

	int iResult = send(client, myUniqueBufferToSend.get(), totalSend, 0);
	if (iResult == SOCKET_ERROR)
	{
		wprintf(L"send failed with error: %d\n", WSAGetLastError());
		closesocket(client);
		WSACleanup();
	}

}

string Server::processRequest(char* bufferPtr)
{
	string firstLine = { "" };
	//extract the first line from buffer
	while (*bufferPtr != '\r') {
		firstLine += *bufferPtr;
		bufferPtr++;
	}
	cout << "Client request: " << firstLine << endl;
	return getFilePath(firstLine);
}

//function to get the file that client want from http request using regex
string Server::getFilePath(string p_toParse)
{
	//static const string REGEX_GET = R"((GET)\s\/(.+)\s(HTTP.+))";

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
void Server::printBuffer(char* bufferPtr, int size)
{

	for (int i = 0; i < size; i++) {
		cout << *bufferPtr;
		bufferPtr++;
	}

}

void Server::printClientPortAndIP(SOCKET client, int port)
{
	//this gets clients ip from sock_addr_in
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int res = getsockname(client, (struct sockaddr *)&addr, &addr_size);
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, clientIp, INET_ADDRSTRLEN);
	string cId = "ID: " + std::to_string(g_clientId) + " ";
	string portStr = "The Client port is: " + std::to_string(port) + " ";
	string clientIP = "The Client IP is: ";
	clientIP.append(clientIp);
	clientIP.append(" ");
	cout << portStr << endl;
	cout << clientIP << endl;

	fwrite(cId.c_str(), sizeof(char), cId.size(), logFile);
	fwrite(portStr.c_str(), sizeof(char), portStr.size(), logFile);
	fwrite(clientIP.c_str(), sizeof(char), clientIP.size(), logFile);
}