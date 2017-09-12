/*
 * HomeHandler.cpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#include "HomeHandler.hpp"
#include <libintl.h>
#define _(String) gettext (String)

void HomeHandler::save(){
	std::ofstream fout(SYSLOG_FILE);
	fout << (*syslog).str();
}

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

	/* if parameter save is present the savelog button was pushed */
	if(CivetServer::getParam(conn, "savelog", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/theaterwagen\">";
	   message = _("Saved!");
       this->save();
	}

	tohead << "<script type=\"text/javascript\">";
	tohead << " $(document).ready(function(){";
	tohead << " var log = $('#syslog'),";
	tohead << " fix = log.html().replace(/(?:\\r\\n|\\r|\\n)/g, '<br />');";
	tohead << " log.html(fix);";
    tohead << "});";
    tohead << "</script>";
	ss << "<form action=\"/theaterwagen\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
    ss << "<br>";
    ss << "</form>";
	ss << "<form action=\"/chasefactory\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"saveall\" value=\"saveall\" id=\"saveall\">" << _("Save All") << "</button>";
	ss << "</form>";
	ss << "<form action=\"/portret\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"portret\" value=\"portret\" id=\"portret\">" << _("Portret") << "</button>";
	ss << "</form>";
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
	ss << "<br>";
	ss << "<h2>";
	ss << "SYSLOG:" << "<br>";
	ss << "<form action=\"/theaterwagen\" method=\"POST\">";
    ss << "<button type=\"submit\" name=\"savelog\" value=\"savelog\" id=\"savelog\">" << _("Save Log") << "</button>";
	ss << "</form>";
	ss << "<br>";
	ss << "<div id=syslog>";
	ss << (*syslog).str();
	ss << "</div>";
	ss << "</h2>";
	ss = getHtml(meta, message, "home",  ss.str().c_str(), tohead.str().c_str());
	mg_printf(conn, "%s", ss.str().c_str());
	return true;
}

HomeHandler::HomeHandler() {
	if (getenv("LANGUAGE") == NULL)
	{
    	setenv("LANGUAGE", "en" , 1);
			  {
			    extern int  _nl_msg_cat_cntr;
			    ++_nl_msg_cat_cntr;
			  }
	}
}

HomeHandler::~HomeHandler() {
	// TODO Auto-generated destructor stub
}

