#pragma once
#include<stdio.h>
#include<iostream>
#include<string>
#include<vector>
#include <fstream>

using std::cin;		using std::cout;
using std::endl;	using std::string;
using std::stoi;

class ServerAPI
{
public:
	ServerAPI();
	ServerAPI(string r);
	string getRequest();
	string getName();
	string getService();
	virtual ~ServerAPI();
private:
	string request; //Ex: APAAA/animals/1
	string name;	//Ex: APAAA
	string service; //Ex: animals
	int    paramId;	//Ex: 1
};

