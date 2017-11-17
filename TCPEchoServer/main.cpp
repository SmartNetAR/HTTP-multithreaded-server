//mainthreadsock define the entry point for console application

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include "server.h"
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
using std::cin;
using std::cout;
using std::string;
//std::mutex g_mutex2;
int main()
{
	Server * server = new Server();
	server->start();
	return 0;
}
