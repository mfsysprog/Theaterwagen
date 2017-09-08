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
	std::string dummy;

	if(CivetServer::getParam(conn, "language", dummy))
		{
		  /* Change language.  */
		  if (std::string(getenv("LANGUAGE")).compare("nl") == 0)
			  setenv ("LANGUAGE", "en", 1);
		  else
			  setenv ("LANGUAGE", "nl", 1);

		  /* Make change known.  */
		  {
		    extern int  _nl_msg_cat_cntr;
		    ++_nl_msg_cat_cntr;
		  }

		   meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"/theaterwagen\"/>";
		   message = "Taal gewijzigd!";
		}

	ss << "<form action=\"/theaterwagen\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"language\" value=\"language\" id=\"language\">Taal wisselen</button>";
	ss << "</form>";
	ss << "<form action=\"/chasefactory\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"saveall\" value=\"saveall\" id=\"saveall\">Alles opslaan</button>";
	ss << "</form>";
	ss << "<form action=\"/portret\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"portret\" value=\"portret\" id=\"portret\">Portret</button>";
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

