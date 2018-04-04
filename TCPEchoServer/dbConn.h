#pragma once
#include <libpq-fe.h>

class dbConn
{
	private:
		char* host;
		char* dataBase;
		char* port;
		char* user;
		char* passwd;
		PGconn *cnn;
		PGresult *result;

	public:
		dbConn();
		dbConn(char* host, char* port, char* dataBase, char* user, char* passwd);
		//bool Connect();
		void Disconnect();
		int Show();
		~dbConn();
};

