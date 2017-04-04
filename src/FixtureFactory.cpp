/*
 * FixtureFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "FixtureFactory.hpp"

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

	char filename[] = CONFIG_FILE;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE);
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
	std::ofstream fout(CONFIG_FILE);
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

	if(CivetServer::getParam(conn, "delete", value))
	{
	   mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
			   	  "text/html\r\nConnection: close\r\n\r\n");
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/fixturefactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->fixturefactory.deleteFixture(atoi(value.c_str()));
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/fixturefactory\" /></head><body>");
		mg_printf(conn, "<h2>Fixture opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->fixturefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/fixturefactory\" /></head><body>");
		mg_printf(conn, "<h2>Fixture ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
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

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << fixture->getUrl() << "\"/></head><body>";
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
	   ss << "<form action=\"/fixturefactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"base\">Base adres:</label>"
	   	     "<input id=\"base\" type=\"text\" size=\"3\" name=\"base\"/>" << "</br>";
	   ss << "<label for=\"number\">Aantal channels:</label>"
	   	   	     "<input id=\"number\" type=\"text\" size=\"3\" name=\"number\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
       ss <<  "</br>";
       ss << "<a href=\"/fixturefactory\">Fixtures</a>";
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
		std::map<int, FixtureFactory::Fixture*>::iterator it = fixturefactory.fixturemap.begin();
		ss << "<h2>Beschikbare fixtures:</h2>";
	    for (std::pair<int, FixtureFactory::Fixture*> element : fixturefactory.fixturemap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->base_channel << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/fixturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool FixtureFactory::Fixture::FixtureHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string value;
	std::string dummy;

	mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");

	if(CivetServer::getParam(conn, "submit", dummy))
	{
		if(CivetServer::getParam(conn,"naam", value))
		  		fixture.naam = value;
		if(CivetServer::getParam(conn,"omschrijving", value))
		  		fixture.omschrijving = value;
		if(CivetServer::getParam(conn,"base", value))
		  		fixture.base_channel = atoi(value.c_str());
		if(CivetServer::getParam(conn,"number", value))
		  		fixture.number_channels = atoi(value.c_str());
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << fixture.getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	} else
	{
		mg_printf(conn, "<h2>&nbsp;</h2>");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	}
	{
		std::stringstream ss;
		ss << "<h2>Fixtures:</h2>";
		ss << "<form action=\"" << fixture.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" value=\"" <<
					  fixture.getNaam() << "\"" << " name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"naam\" type=\"text\" value=\"" <<
					  fixture.getOmschrijving() << "\"" << " name=\"omschrijving\"/>" << "</br>";
		ss << "<label for=\"base\">Base adres:</label>"
			  "<input id=\"base\" type=\"text\" value=\"" <<
			  fixture.base_channel << "\"" << " name=\"base\"/>" << "</br>";
		ss << "<label for=\"number\">Aantal channels:</label>"
			  "<input id=\"number\" type=\"text\" value=\"" <<
			  fixture.number_channels << "\"" << " name=\"number\"/>" << "</br>";
		ss << "</br>";
		ss << "</br>";
		ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
		ss <<  "</br>";
	    ss << "</form>";
		ss <<  "</br>";
		ss << "<a href=\"/fixturefactory\">Fixtures</a>";
		ss <<  "</br>";
		ss << "<a href=\"/scenefactory\">Scenes</a>";
		ss <<  "</br>";
		ss << "<a href=\"/\">Home</a>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	return true;
}



