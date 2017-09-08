/*
 * HomeHandler.cpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#include "HomeHandler.hpp"
#include <libintl.h>
#define _(String) gettext (String)

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
	std::string value;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "save", dummy))
		{
		  CivetServer::getParam(conn,"taal", value);

		  /* Change language.  */
		  setenv ("LANGUAGE", value.c_str() , 1);

		  /* Make change known.  */
		  {
		    extern int  _nl_msg_cat_cntr;
		    ++_nl_msg_cat_cntr;
		  }
		}

	ss << "<h2>" << _("Language") << ":";
	ss << "<form action=\"/theaterwagen\" method=\"POST\">";
    ss << "<select id=\"taal\" name=\"taal\">";
    if (std::string(getenv("LANGUAGE")).compare("en") == 0)
    	ss << "<option selected=\"selected\" value=\"en\">English</option>";
    else
    	ss << "<option value=\"en\">English</option>";
    if (std::string(getenv("LANGUAGE")).compare("nl") == 0)
    	ss << "<option selected=\"selected\" value=\"nl\">Nederlands</option>";
    else
    	ss << "<option value=\"nl\">Nederlands</option>";
    ss << "</select><br>";
    ss << "<button type=\"submit\" name=\"save\" value=\"save\" id=\"save\">" << _("Change Language") << "</button>";
	ss << "</form>";
	ss << "</h2>";
	ss << "<form action=\"/chasefactory\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"saveall\" value=\"saveall\" id=\"saveall\">" << _("Save All") << "</button>";
	ss << "</form>";
	ss << "<form action=\"/portret\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"portret\" value=\"portret\" id=\"portret\">" << _("Portret") << "</button>";
	ss << "</form>";
	ss = getHtml(meta, message, "home",  ss.str().c_str(), tohead.str().c_str());
	mg_printf(conn, "%s", ss.str().c_str());
	return true;
}

HomeHandler::HomeHandler() {
	if (getenv("LANGUAGE") == NULL) setenv("LANGUAGE", "en" , 1);
}

HomeHandler::~HomeHandler() {
	// TODO Auto-generated destructor stub
}

