/*
 * FileHandler.cpp
 *
 * The filehandler imbeds an iframe to a richfilemanager, which is installed in the filemanager
 * directory under the main installation directory.
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#include "FileHandler.hpp"

bool FileHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}

bool FileHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}

bool FileHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	ss << "<iframe src=\"/filemanager/\" style=\"width:100%\">Your Browser doesn't support Iframes</iframe>";
	ss = getHtml(meta, message, "files",  ss.str().c_str());
	mg_printf(conn, "%s", ss.str().c_str());
	return true;
}

FileHandler::FileHandler() {
}

FileHandler::~FileHandler() {
	// TODO Auto-generated destructor stub
}

