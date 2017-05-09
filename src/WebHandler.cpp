
#include "WebHandler.hpp"

WebHandler::WebHandler(){
	server->addHandler("/portret", this);
}

WebHandler::~WebHandler(){
}

bool WebHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return WebHandler::handleAll("GET", server, conn);
	}

bool WebHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return WebHandler::handleAll("POST", server, conn);
	}

void WebHandler::setVoor(){
	portret = "voor";
}

void WebHandler::setNa(){
	portret = "na";
}

bool WebHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string dummy;
	std::string value;
	{
		mg_printf(conn,
			          "HTTP/1.1 200 OK\r\nContent-Type: "
			          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
	    ss << "<html><head><meta charset=\"UTF-8\"><meta http-equiv=\"refresh\" content=\"5;url=/portret\" />";
	    ss << "<style>";
	    ss << "body, html {";
	    ss << "height: 100%;";
	    ss << "margin: 0;";
	    ss << "}";
	    ss << ".bg {";
	    ss << "    background-image: url(\"images/portret_" << portret << ".jpg\");";
	    ss << "    height: 100%;";
	    ss << "    background-position: center;";
	    ss << "    background-repeat: no-repeat;";
	    ss << "    background-size: cover;";
	    ss << "}";
	    ss << "</style>";
	    ss << "</head>";
	    ss << "<body>";
	    ss << "<div class=\"bg\"></div>";
		ss << "</body></html>";
	    mg_printf(conn, ss.str().c_str(),"%s");
	}

	return true;
}

