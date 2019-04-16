/*
 * FileHandler.hpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#ifndef FILEHANDLER_H_
#define FILEHANDLER_H_

#include "Theaterwagen.hpp"
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <CivetServer.h>
#include <unistd.h>
#include <cstring>

class FileHandler : public CivetHandler {
public:
	FileHandler();
	~FileHandler();
	bool handleGet(CivetServer *server, struct mg_connection *conn);
	bool handlePost(CivetServer *server, struct mg_connection *conn);
private:
	bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
};


#endif /* FILEHANDLER_HPP */
