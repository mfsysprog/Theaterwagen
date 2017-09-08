/*
 * ToggleFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "ToggleFactory.hpp"
#include <libintl.h>
#define _(String) gettext (String)

/*
 * ToggleFactory Constructor en Destructor
 */
ToggleFactory::ToggleFactory(){
    mfh = new ToggleFactory::ToggleFactoryHandler(*this);
	server->addHandler("/togglefactory", mfh);
	load();
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

	char filename[] = CONFIG_FILE_TOGGLE;
	std::fstream file;
	file.open(filename, std::fstream::in | std::fstream::out | std::fstream::app);
	/* als bestand nog niet bestaat, dan leeg aanmaken */
	if (!file)
	{
		file.open(filename,  std::fstream::in | std::fstream::out | std::fstream::trunc);
        file <<"\n";
        file.close();
	}
	else file.close();

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_TOGGLE);
	for (std::size_t i=0;i<node.size();i++) {
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
	std::ofstream fout(CONFIG_FILE_TOGGLE);
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
	delay(500);
}

void ToggleFactory::Toggle::Start(){
	digitalWrite(relay, LOW);
	delay(500);
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
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/togglefactory\" />";
	   this->togglefactory.deleteToggle(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/togglefactory\" />";
	   message = _("Saved!");
       this->togglefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/togglefactory\" />";
	    message = _("Loaded!");
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

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + toggle->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/togglefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << ":</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"relay\">" << _("Toggle") << " GPIO:</label>"
	   	     "<input class=\"inside\" id=\"relay\" type=\"text\" size=\"3\" name=\"relay\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";
   	   ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
       ss <<  "</br>";
	}
	else
	/* initial page display */
	{
		std::map<std::string, ToggleFactory::Toggle*>::iterator it = togglefactory.togglemap.begin();
	    for (std::pair<std::string, ToggleFactory::Toggle*> element : togglefactory.togglemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/togglefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "toggle",  ss.str().c_str());
    mg_printf(conn, ss.str().c_str(),"%s");
	return true;
}

bool ToggleFactory::Toggle::ToggleHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string param = "chan";
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;
	std::string value;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		toggle.naam = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "omschrijving", value))
	{
		CivetServer::getParam(conn,"value", value);
		toggle.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "relay", value))
	{
		CivetServer::getParam(conn,"value", value);
		toggle.relay = atoi(value.c_str());
		toggle.Initialize();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	/* if parameter start is present start button was pushed */
	if(CivetServer::getParam(conn, "start", dummy))
	{
		toggle.Start();
        meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + toggle.getUrl() + "\"/>";
        message = _("Toggle on!");
	}
	/* if parameter stop is present stop button was pushed */
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		toggle.Stop();
        meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + toggle.getUrl() + "\"/>";
        message = _("Toggle off!");
	}

	/* initial page display */
	{
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << toggle.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << toggle.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#relay').on('change', function() {";
		tohead << " $.get( \"" << toggle.getUrl() << "\", { relay: 'true', value: $('#relay').val() }, function( data ) {";
		tohead << "  $( \"#button\" ).html( data );})";
	    tohead << "});";
		tohead << "});";
		tohead << "</script>";
		ss << "<form action=\"" << toggle.getUrl() << "\" method=\"POST\">";
		ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
		ss << "<br>";
		ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">" << _("On") << "</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">" << _("Off") << "</button>";
	    ss << "</form>";
	    ss << "<h2>";
	    ss << _("Current State") << ":<br>";
	    ss << _("Current State") << " " << _("Toggle") << ":&nbsp;" << digitalRead(toggle.relay) << "<br>";
	    ss << "</h2>";
	    ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  toggle.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  toggle.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<label for=\"relay\">" << _("Toggle") << " GPIO:</label>"
	    	  "<input class=\"inside\" id=\"relay\" type=\"text\" size=\"4\" value=\"" <<
	    	  toggle.relay << "\" name=\"relay\"/>" << "</br>";
	    ss << "<br>";
	    ss << "</div>";
	    ss << "<br>";
	    ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";

	}

	ss = getHtml(meta, message, "toggle",  ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(),"%s");
	return true;
}

