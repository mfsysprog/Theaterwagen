/*
 * ChaseFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "ChaseFactory.hpp"

/*
 * ChaseFactory Constructor en Destructor
 */
ChaseFactory::ChaseFactory(){
    mfh = new ChaseFactory::ChaseFactoryHandler(*this);
	server->addHandler("/chasefactory", mfh);
	load();

	fixture = new FixtureFactory();
	scene = new SceneFactory(fixture);
	music = new MusicFactory();
	sound = new SoundFactory();
	motor = new MotorFactory();
	toggle = new ToggleFactory();
}

ChaseFactory::~ChaseFactory(){
	delete mfh;
	std::map<std::string, ChaseFactory::Chase*>::iterator it = chasemap.begin();
	if (it != chasemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    chasemap.erase(it);
	}
	delete fixture;
	delete scene;
	delete music;
	delete sound;
	delete motor;
	delete toggle;
}

/*
 * ChaseFactoryHandler Constructor en Destructor
 */
ChaseFactory::ChaseFactoryHandler::ChaseFactoryHandler(ChaseFactory& chasefactory):chasefactory(chasefactory){
}


ChaseFactory::ChaseFactoryHandler::~ChaseFactoryHandler(){
}

/*
 * Chase Constructor en Destructor
 */
ChaseFactory::Chase::Chase(ChaseFactory& cf, std::string naam, std::string omschrijving):cf(cf){
	mh = new ChaseFactory::Chase::ChaseHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;

	sequence_list = new std::list<sequence_item>();
	sequence_item scene1 = {"Scene::Play","3fb4e958-48a6-4b8d-b556-f44944c4e156",5000};
	sequence_list->push_back(scene1);
	sequence_item scene2 = {"Scene::Play","936491c6-8bb2-4bd0-a512-594eda61a9bc",2000};
	sequence_list->push_back(scene2);
	sequence_item scene3 = {"Scene::Play","a6b424a9-54ac-4b1d-b2ab-def8b58e50ec",3000};
	sequence_list->push_back(scene3);

	Start();

	std::stringstream ss;
	ss << "/chase-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ChaseFactory::Chase::Chase(ChaseFactory& cf, std::string uuidstr, std::string naam, std::string omschrijving):cf(cf){
	mh = new ChaseFactory::Chase::ChaseHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;

	sequence_list = new std::list<sequence_item>();
	sequence_item scene1 = {"Scene::Play","3fb4e958-48a6-4b8d-b556-f44944c4e156",5000};
	sequence_list->push_back(scene1);
	sequence_item scene2 = {"Scene::Play","936491c6-8bb2-4bd0-a512-594eda61a9bc",2000};
	sequence_list->push_back(scene2);
	sequence_item scene3 = {"Scene::Play","a6b424a9-54ac-4b1d-b2ab-def8b58e50ec",3000};
	sequence_list->push_back(scene3);

	Start();

	std::stringstream ss;
	ss << "/chase-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ChaseFactory::Chase::~Chase(){
	delete mh;
}

/*
 * Chase Handler Constructor en Destructor
 */
ChaseFactory::Chase::ChaseHandler::ChaseHandler(ChaseFactory::Chase& chase):chase(chase){
}

ChaseFactory::Chase::ChaseHandler::~ChaseHandler(){
}


/* overige functies
 *
 */

void ChaseFactory::load(){
	for (std::pair<std::string, ChaseFactory::Chase*> element  : chasemap)
	{
		deleteChase(element.first);
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
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		ChaseFactory::Chase * chase = new ChaseFactory::Chase(*this, uuidstr, naam, omschrijving);
		std::string uuid_str = chase->getUuid();
		chasemap.insert(std::make_pair(uuid_str,chase));
	}
}

void ChaseFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, ChaseFactory::Chase*>::iterator it = chasemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, ChaseFactory::Chase*> element  : chasemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

std::string ChaseFactory::Chase::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string ChaseFactory::Chase::getUrl(){
	return url;
}

ChaseFactory::Chase* ChaseFactory::addChase(std::string naam, std::string omschrijving){
	ChaseFactory::Chase * chase = new ChaseFactory::Chase(*this, naam, omschrijving);
	std::string uuid_str = chase->getUuid();
	chasemap.insert(std::make_pair(uuid_str,chase));
	return chase;
}

void ChaseFactory::deleteChase(std::string uuid){
	std::map<std::string, ChaseFactory::Chase*>::iterator it = chasemap.begin();
    it = chasemap.find(uuid);
	if (it != chasemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    chasemap.erase(it);
	}
}

void ChaseFactory::Chase::Stop(){
	delay(1000);
}

void ChaseFactory::Chase::Start(){

	while (true)
	{
		std::list<sequence_item>::const_iterator it;
		for (it = sequence_list->begin(); it != sequence_list->end(); ++it)
		{
			cf.scene->scenemap.find((*it).uuid)->second->Play();
			delay((*it).millisecond);
		}
	};
}

std::string ChaseFactory::Chase::getNaam(){
	return naam;
}

std::string ChaseFactory::Chase::getOmschrijving(){
	return omschrijving;
}

bool ChaseFactory::ChaseFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ChaseFactory::ChaseFactoryHandler::handleAll("GET", server, conn);
	}

bool ChaseFactory::ChaseFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ChaseFactory::ChaseFactoryHandler::handleAll("POST", server, conn);
	}

bool ChaseFactory::Chase::ChaseHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ChaseFactory::Chase::ChaseHandler::handleAll("GET", server, conn);
	}

bool ChaseFactory::Chase::ChaseHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ChaseFactory::Chase::ChaseHandler::handleAll("POST", server, conn);
	}

bool ChaseFactory::ChaseFactoryHandler::handleAll(const char *method,
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
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/chasefactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->chasefactory.deleteChase(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/chasefactory\" /></head><body>");
		mg_printf(conn, "<h2>Chase opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->chasefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/chasefactory\" /></head><body>");
		mg_printf(conn, "<h2>Chase ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->chasefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;

		ChaseFactory::Chase* chase = chasefactory.addChase(naam, omschrijving);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << chase->getUrl() << "\"/></head><body>";
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
	   ss << "<form action=\"/chasefactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
   	   ss << "<a href=\"/chasefactory\">Chases</a>";
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
		std::map<std::string, ChaseFactory::Chase*>::iterator it = chasefactory.chasemap.begin();
		ss << "<h2>Beschikbare Chases:</h2>";
	    for (std::pair<std::string, ChaseFactory::Chase*> element : chasefactory.chasemap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool ChaseFactory::Chase::ChaseHandler::handleAll(const char *method,
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
	   CivetServer::getParam(conn,"naam", s[0]);
	   chase.naam = s[0].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[1]);
	   chase.omschrijving = s[1].c_str();

	   std::stringstream ss;
	   ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/></head><body>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "start", dummy))
	{
		chase.Start();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Starten...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		chase.Stop();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/></head><body>";
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
		ss << "<h2>Chases:</h2>";
		ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  chase.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  chase.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">START</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
	    ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<a href=\"/chasefactory\">Chases</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

