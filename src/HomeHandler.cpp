/*
 * HomeHandler.cpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#include "HomeHandler.hpp"

bool HomeHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}

bool HomeHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}

bool HomeHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	ss << "<form action=\"/chasefactory\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"saveall\" value=\"saveall\" id=\"saveall\">Alles opslaan</button>";
	ss << "</form>";
	ss = getHtml(meta, message, "home",  ss.str().c_str());
	mg_printf(conn, "%s", ss.str().c_str());
	return true;
}

HomeHandler::HomeHandler() {
}

HomeHandler::~HomeHandler() {
	// TODO Auto-generated destructor stub
}
