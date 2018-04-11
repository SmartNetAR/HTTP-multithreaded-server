#include "ServerAPI.h"



ServerAPI::ServerAPI()
{
}

ServerAPI::ServerAPI(string r):request(r)
{
	//request Example = APAAA/animals/1
	string requestAux;

	requestAux = request; //requestAux = APAAA/animals/1
	name       = requestAux.substr(0, requestAux.find("/", 0)); //requestAux.name = APAAA 
	requestAux = requestAux.substr(requestAux.find("/", 0) + 1); //requestAux = animals/1
	service    = requestAux.substr(0, requestAux.find("/", 0)); //requestAux.service = animals
	requestAux = requestAux.substr(requestAux.find("/", 0) + 1); //requestAux = 1
	paramId    = stoi(requestAux); //realizar clase parametros
}

ServerAPI::~ServerAPI()
{
}

string ServerAPI::getRequest()
{
	return request;
}
string ServerAPI::getName()
{
	return name;
}
string ServerAPI::getService()
{
	return service;
}

