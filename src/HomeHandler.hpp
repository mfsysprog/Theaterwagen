/*
 * HomeHandler.hpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#ifndef HOMEHANDLER_H_
#define HOMEHANDLER_H_

#include "Theaterwagen.hpp"
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include "CivetServer.h"
#include <unistd.h>
#include <cstring>

#define SYSLOG_FILE RESOURCES_DIR "syslog.txt"

extern std::stringstream* syslog;

class HomeHandler : public CivetHandler {
public:
	HomeHandler();
	~HomeHandler();
	bool handleGet(CivetServer *server, struct mg_connection *conn);
	bool handlePost(CivetServer *server, struct mg_connection *conn);
private:
	bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
	void save();
};


#endif /* HOMEHANDLER_HPP */
