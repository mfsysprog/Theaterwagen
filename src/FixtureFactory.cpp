/*
 * FixtureFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "FixtureFactory.hpp"
#include <libintl.h>
#define _(String) gettext (String)
/*
 * FixtureFactory Constructor en Destructor
 */
FixtureFactory::FixtureFactory(){
    mfh = new FixtureFactory::FixtureFactoryHandler(*this);
	server->addHandler("/fixturefactory", mfh);
	load();
}

FixtureFactory::~FixtureFactory(){
	delete mfh;
	std::map<int, FixtureFactory::Fixture*>::iterator it = fixturemap.begin();
	if (it != fixturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    fixturemap.erase(it);
	}
}

/*
 * FixtureFactoryHandler Constructor en Destructor
 */
FixtureFactory::FixtureFactoryHandler::FixtureFactoryHandler(FixtureFactory& fixturefactory):fixturefactory(fixturefactory){
}


FixtureFactory::FixtureFactoryHandler::~FixtureFactoryHandler(){
}

/*
 * Fixture Constructor en Destructor
 */
FixtureFactory::Fixture::Fixture(std::string naam, std::string omschrijving, int base_channel, int number_channels){
	mh = new FixtureFactory::Fixture::FixtureHandler(*this);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->number_channels = number_channels;
	this->base_channel = base_channel;
	std::stringstream ss;
	ss << "/fixture-" << this->base_channel;
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

FixtureFactory::Fixture::~Fixture(){
	delete mh;
}

/*
 * Fixture Handler Constructor en Destructor
 */
FixtureFactory::Fixture::FixtureHandler::FixtureHandler(FixtureFactory::Fixture& fixture):fixture(fixture){
}

FixtureFactory::Fixture::FixtureHandler::~FixtureHandler(){
}


/* overige functies
 *
 */

void FixtureFactory::load(){
	for (std::pair<int, FixtureFactory::Fixture*> element  : fixturemap)
	{
		deleteFixture(element.first);
	}

	char filename[] = CONFIG_FILE_FIXTURE;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_FIXTURE);
	for (std::size_t i=0;i<node.size();i++) {
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		int base = node[i]["base"].as<int>();
		int number = node[i]["number"].as<int>();
		FixtureFactory::Fixture * fixture = new FixtureFactory::Fixture(naam, omschrijving, base, number);
		fixturemap.insert(std::make_pair(base,fixture));
	}
}

void FixtureFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_FIXTURE);
	std::map<int, FixtureFactory::Fixture*>::iterator it = fixturemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<int, FixtureFactory::Fixture*> element  : fixturemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "base";
		emitter << YAML::Value << element.second->base_channel;
		emitter << YAML::Key << "number";
		emitter << YAML::Value << element.second->number_channels;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

std::string FixtureFactory::Fixture::getUrl(){
	return url;
}

FixtureFactory::Fixture* FixtureFactory::addFixture(std::string naam, std::string omschrijving, int base_channel, int number_channels){
	FixtureFactory::Fixture * fixture = new FixtureFactory::Fixture(naam, omschrijving, base_channel, number_channels);
	fixturemap.insert(std::make_pair(base_channel,fixture));
	return fixture;
}

void FixtureFactory::deleteFixture(int base){
	std::map<int, FixtureFactory::Fixture*>::iterator it = fixturemap.begin();
    it = fixturemap.find(base);
	if (it != fixturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    fixturemap.erase(it);
	}
}

std::string FixtureFactory::Fixture::getNaam(){
	return naam;
}

std::string FixtureFactory::Fixture::getOmschrijving(){
	return omschrijving;
}


bool FixtureFactory::FixtureFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return FixtureFactory::FixtureFactoryHandler::handleAll("GET", server, conn);
	}

bool FixtureFactory::FixtureFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return FixtureFactory::FixtureFactoryHandler::handleAll("POST", server, conn);
	}

bool FixtureFactory::Fixture::FixtureHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return FixtureFactory::Fixture::FixtureHandler::handleAll("GET", server, conn);
	}

bool FixtureFactory::Fixture::FixtureHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return FixtureFactory::Fixture::FixtureHandler::handleAll("POST", server, conn);
	}

bool FixtureFactory::FixtureFactoryHandler::handleAll(const char *method,
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
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/fixturefactory\" />";
	   this->fixturefactory.deleteFixture(atoi(value.c_str()));
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/fixturefactory\" />";
	   message = _("Saved!");
       this->fixturefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/fixturefactory\" />";
	   message = _("Loaded!");
       this->fixturefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "base", value);
		int base = atoi(value.c_str());
		CivetServer::getParam(conn, "number", value);
		int number = atoi(value.c_str());

		FixtureFactory::Fixture* fixture = fixturefactory.addFixture(naam, omschrijving, base, number);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + fixture->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/fixturefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << "</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << "</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"base\">" << _("Base address") << ":</label>"
	   	     "<input class=\"inside\" id=\"base\" type=\"number\" min=\"0\" max=\"255\" placeholder=\"0\" step=\"1\" name=\"base\"/>" << "</br>";
	   ss << "<label for=\"number\">" <<_("Number of channels") << ":</label>"
	   	   	 "<input class=\"inside\" id=\"number\" type=\"number\" min=\"1\" max=\"256\" placeholder=\"1\" step=\"1\" name=\"number\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";
	}
	else
	/* initial page display */
	{
		std::map<int, FixtureFactory::Fixture*>::iterator it = fixturefactory.fixturemap.begin();
	    for (std::pair<int, FixtureFactory::Fixture*> element : fixturefactory.fixturemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->base_channel << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "fixture",  ss.str().c_str());
    mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool FixtureFactory::Fixture::FixtureHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string value;
	std::string dummy;
	std::string message="&nbsp;";
	std::string meta="";
    std::stringstream ss;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		fixture.naam = value.c_str();
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
		fixture.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "base", value))
	{
		CivetServer::getParam(conn,"value", value);
		fixture.base_channel = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "number", value))
	{
		CivetServer::getParam(conn,"value", value);
		fixture.number_channels = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	{
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << fixture.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << fixture.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#base').on('change', function() {";
		tohead << " $.get( \"" << fixture.getUrl() << "\", { base: 'true', value: $('#base').val() }, function( data ) {";
		tohead << "  $( \"#base\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#number').on('change', function() {";
		tohead << " $.get( \"" << fixture.getUrl() << "\", { number: 'true', value: $('#number').val() }, function( data ) {";
		tohead << "  $( \"#number\" ).html( data );})";
	    tohead << "});";
	    tohead << "});";
		tohead << "</script>";
		ss << "<form action=\"" << fixture.getUrl() << "\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
	    ss <<  "<br>";
	    ss << "</form>";
		ss << "<h2>";
		ss << "&nbsp;";
		ss << "</h2>";
		 ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" value=\"" <<
					  fixture.getNaam() << "\"" << " name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" value=\"" <<
					  fixture.getOmschrijving() << "\"" << " name=\"omschrijving\"/>" << "</br>";
		ss << "<label for=\"base\">" << _("Base address") << ":</label>"
			  "<input class=\"inside\" id=\"base\" type=\"number\" min=\"0\" max=\"255\" placeholder=\"0\" step=\"1\" value=\"" <<
			  fixture.base_channel << "\"" << " name=\"base\"/>" << "</br>";
		ss << "<label for=\"number\">" << _("Number of channels") << ":</label>"
			  "<input class=\"inside\" id=\"number\" type=\"number\" min=\"1\" max=\"256\" placeholder=\"1\" step=\"1\" value=\"" <<
			  fixture.number_channels << "\"" << " name=\"number\"/>" << "</br>";
		ss << "</div>";
		ss << "<br>";
		ss <<  "<br>";
	}

	ss = getHtml(meta, message, "fixture",  ss.str().c_str(), tohead.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}



