
#ifndef WEBHANDLER_HPP_
#define WEBHANDLER_HPP_

#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include <unistd.h>
#include <cstring>
#include <dirent.h>

extern CivetServer* server;

class WebHandler : public CivetHandler
	{
		public:
		WebHandler();
		~WebHandler();
		void setVoor();
		void setNa();
		private:
		std::string portret = "voor";
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
	};

#endif /* TOGGLEFACTORY_HPP_ */
