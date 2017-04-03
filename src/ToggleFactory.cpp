/*
 * ToggleFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "ToggleFactory.hpp"

/*
 * ToggleFactory Constructor en Destructor
 */
ToggleFactory::ToggleFactory(){
    mfh = new ToggleFactory::ToggleFactoryHandler(*this);
	server->addHandler("/togglefactory", mfh);
}

ToggleFactory::~ToggleFactory(){
	delete mfh;
	std::map<std::string, ToggleFactory::Toggle*>::iterator it = togglemap.begin();
	if (it != togglemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    togglemap.erase(it);
	}
}

/*
 * ToggleFactoryHandler Constructor en Destructor
 */
ToggleFactory::ToggleFactoryHandler::ToggleFactoryHandler(ToggleFactory& togglefactory):togglefactory(togglefactory){
}


ToggleFactory::ToggleFactoryHandler::~ToggleFactoryHandler(){
}

/*
 * Toggle Constructor en Destructor
 */
ToggleFactory::Toggle::Toggle(std::string naam, std::string omschrijving, int GPIO_relay){
	mh = new ToggleFactory::Toggle::ToggleHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;

	/*
	 * initialise gpio
	 */
	relay = GPIO_relay;

	Initialize();

	std::stringstream ss;
	ss << "/toggle-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ToggleFactory::Toggle::Toggle(std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_relay){
	mh = new ToggleFactory::Toggle::ToggleHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;

	/*
	 * initialise gpio
	 */
	relay = GPIO_relay;

	Initialize();

	std::stringstream ss;
	ss << "/toggle-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ToggleFactory::Toggle::~Toggle(){
	delete mh;
}

/*
 * Toggle Handler Constructor en Destructor
 */
ToggleFactory::Toggle::ToggleHandler::ToggleHandler(ToggleFactory::Toggle& toggle):toggle(toggle){
}

ToggleFactory::Toggle::ToggleHandler::~ToggleHandler(){
}


/* overige functies
 *
 */

void ToggleFactory::load(){
	for (std::pair<std::string, ToggleFactory::Toggle*> element  : togglemap)
	{
		deleteToggle(element.first);
	}
	YAML::Node node = YAML::LoadFile(CONFIG_FILE);
	assert(node.IsSequence());
	for (std::size_t i=0;i<node.size();i++) {
		assert(node[i].IsMap());
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		int relay = node[i]["relay"].as<int>();
		ToggleFactory::Toggle * toggle = new ToggleFactory::Toggle(uuidstr, naam, omschrijving, relay);
		std::string uuid_str = toggle->getUuid();
		togglemap.insert(std::make_pair(uuid_str,toggle));
	}
}

void ToggleFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, ToggleFactory::Toggle*>::iterator it = togglemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, ToggleFactory::Toggle*> element  : togglemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "relay";
		emitter << YAML::Value << element.second->relay;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void ToggleFactory::Toggle::Initialize(){
	/*
	 * set relay to output and full stop
	 */
	pinMode(relay, OUTPUT);
	digitalWrite(relay, HIGH);
}

std::string ToggleFactory::Toggle::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string ToggleFactory::Toggle::getUrl(){
	return url;
}

ToggleFactory::Toggle* ToggleFactory::addToggle(std::string naam, std::string omschrijving, int GPIO_relay){
	ToggleFactory::Toggle * toggle = new ToggleFactory::Toggle(naam, omschrijving, GPIO_relay);
	std::string uuid_str = toggle->getUuid();
	togglemap.insert(std::make_pair(uuid_str,toggle));
	return toggle;
}

void ToggleFactory::deleteToggle(std::string uuid){
	std::map<std::string, ToggleFactory::Toggle*>::iterator it = togglemap.begin();
    it = togglemap.find(uuid);
	if (it != togglemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    togglemap.erase(it);
	}
}

void ToggleFactory::Toggle::Stop(){
	digitalWrite(relay, HIGH);
	delay(1000);
}

void ToggleFactory::Toggle::Start(){
	digitalWrite(relay, LOW);
	delay(1000);
}

std::string ToggleFactory::Toggle::getNaam(){
	return naam;
}

std::string ToggleFactory::Toggle::getOmschrijving(){
	return omschrijving;
}

bool ToggleFactory::ToggleFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ToggleFactory::ToggleFactoryHandler::handleAll("GET", server, conn);
	}

bool ToggleFactory::ToggleFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ToggleFactory::ToggleFactoryHandler::handleAll("POST", server, conn);
	}

bool ToggleFactory::Toggle::ToggleHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ToggleFactory::Toggle::ToggleHandler::handleAll("GET", server, conn);
	}

bool ToggleFactory::Toggle::ToggleHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ToggleFactory::Toggle::ToggleHandler::handleAll("POST", server, conn);
	}

bool ToggleFactory::ToggleFactoryHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string dummy;
	std::string value;

	if(CivetServer::getParam(conn, "delete", value))
	{
	   mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
			   	  "text/html\r\nConnection: close\r\n\r\n");
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/togglefactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->togglefactory.deleteToggle(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/togglefactory\" /></head><body>");
		mg_printf(conn, "<h2>Toggle opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->togglefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/togglefactory\" /></head><body>");
		mg_printf(conn, "<h2>Toggle ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->togglefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "relay", value);
		int relay = atoi(value.c_str());

		ToggleFactory::Toggle* toggle = togglefactory.addToggle(naam, omschrijving, relay);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << toggle->getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	else if(CivetServer::getParam(conn, "new", dummy))
	{
       mg_printf(conn,
		        "HTTP/1.1 200 OK\r\nContent-Type: "
		         "text/html\r\nConnection: close\r\n\r\n");
       mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	   std::stringstream ss;
	   ss << "<form action=\"/togglefactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"relay\">Relais GPIO:</label>"
	   	     "<input id=\"relay\" type=\"text\" size=\"3\" name=\"relay\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
   	   ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
       ss <<  "</br>";
       ss << "<a href=\"/togglefactory\">Togglees</a>";
       ss <<  "</br>";
       ss << "<a href=\"/\">Home</a>";
       mg_printf(conn, ss.str().c_str());
       mg_printf(conn, "</body></html>");
	}
	/* initial page display */
	else
	{
		mg_printf(conn,
			          "HTTP/1.1 200 OK\r\nContent-Type: "
			          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
		std::stringstream ss;
		std::map<std::string, ToggleFactory::Toggle*>::iterator it = togglefactory.togglemap.begin();
		ss << "<h2>Beschikbare Aan/Uit:</h2>";
	    for (std::pair<std::string, ToggleFactory::Toggle*> element : togglefactory.togglemap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool ToggleFactory::Toggle::ToggleHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string param = "chan";
	mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n");

	/* if parameter submit is present the submit button was pushed */
	if(CivetServer::getParam(conn, "submit", dummy))
	{
	   CivetServer::getParam(conn,"relay", s[0]);
	   toggle.relay = atoi(s[0].c_str());
	   CivetServer::getParam(conn,"naam", s[1]);
	   toggle.naam = s[1].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[2]);
	   toggle.omschrijving = s[2].c_str();

	   toggle.Initialize();

	   std::stringstream ss;
	   ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << toggle.getUrl() << "\"/></head><body>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "start", dummy))
	{
		toggle.Start();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << toggle.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Starten...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		toggle.Stop();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << toggle.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stoppen...!</h2>");
	}
	else
	{
		mg_printf(conn, "<h2>&nbsp;</h2>");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	}
	/* initial page display */
	{
		std::stringstream ss;
		ss << "<h2>Aan/Uit:</h2>";
		ss << "<form action=\"" << toggle.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  toggle.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  toggle.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
	    ss << "Huidige status relais:&nbsp;" << digitalRead(toggle.relay) << "<br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">START</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
		ss << "<h2>GPIO pin:</h2>";
	    ss << "<label for=\"relay\">Right Relay GPIO pin:</label>"
	    	  "<input id=\"relay\" type=\"text\" size=\"4\" value=\"" <<
	    	  toggle.relay << "\" name=\"relay\"/>" << "</br>";
	    ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
	    ss << "<a href=\"/togglefactory\">Aan/Uit</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

