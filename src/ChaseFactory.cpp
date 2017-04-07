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
	this->running = false;

	sequence_list = new std::list<sequence_item>();

	std::stringstream ss;
	ss << "/chase-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ChaseFactory::Chase::Chase(ChaseFactory& cf, std::string uuidstr, std::string naam, std::string omschrijving, std::list<sequence_item>* sequence_list):cf(cf){
	mh = new ChaseFactory::Chase::ChaseHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->running = false;

	this->sequence_list = sequence_list;

	std::stringstream ss;
	ss << "/chase-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ChaseFactory::Chase::~Chase(){
	delete mh;
	delete sequence_list;
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
		std::list<sequence_item>* sequence_list = new std::list<sequence_item>();
		for (std::size_t k=0;k<node[i]["actions"].size();k += 2) {
			sequence_item item = {node[i]["actions"][k].as<std::string>(), node[i]["actions"][k+1].as<std::string>()};
			sequence_list->push_back(item);
		}
		ChaseFactory::Chase * chase = new ChaseFactory::Chase(*this, uuidstr, naam, omschrijving, sequence_list);
		std::string uuid_str = chase->getUuid();
		chasemap.insert(std::make_pair(uuid_str,chase));
	}
}

void ChaseFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, ChaseFactory::Chase*>::iterator it = chasemap.begin();
	std::list<sequence_item>::const_iterator it_list;

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
		emitter << YAML::Key << "actions";
		emitter << YAML::Flow;
		emitter << YAML::BeginSeq;
		for (it_list = element.second->sequence_list->begin(); it_list != element.second->sequence_list->end(); ++it_list)
		{
			emitter << (*it_list).action << (*it_list).uuid_or_milliseconds;
		}
		emitter << YAML::EndSeq;
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
	this->running = false;
}

void ChaseFactory::Chase::Start(){
	if (!this->running)
	{
		this->running = true;
		std::thread( [this] { Action(); } ).detach();
	}
}

void ChaseFactory::Chase::Action()
{
	std::cout << "Running chase!" << std::endl;
	std::list<sequence_item>::iterator it;
	for (it = sequence_list->begin(); it != sequence_list->end(); ++it)
	{
		if (!this->running) break;
		std::string::size_type pos = (*it).action.find('::');
		std::string action = (*it).action.substr(0,pos);
		std::string method = (*it).action.substr(pos+2,(*it).action.length()-pos-2);

		/* skippen van ongeldige verwijzigingen */
		if ((*it).invalid) continue;

		(*it).active = true;
		if (action.compare("Aan/Uit") == 0)
		{
			if (method.compare("Aan") == 0)
		      cf.toggle->togglemap.find((*it).uuid_or_milliseconds)->second->Start();
			if (method.compare("Uit") == 0)
		      cf.toggle->togglemap.find((*it).uuid_or_milliseconds)->second->Stop();
		}
		if (action.compare("Geluid") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->Play();
			if (method.compare("Stop") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->Stop();
		}
		if (action.compare("Motor") == 0)
		{
			if (method.compare("Links") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Start(LEFT);
			if (method.compare("Rechts") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Start(RIGHT);
			if (method.compare("Stop") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Stop();
			if (method.compare("Wachten") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Wait();
		}
		if (action.compare("Muziek") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->Play();
			if (method.compare("Stop") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->Stop();
		}
		if (action.compare("Scene") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.scene->scenemap.find((*it).uuid_or_milliseconds)->second->Play();
		}
		if (action.compare("Tijd") == 0)
		{
			delay(atoi((*it).uuid_or_milliseconds.c_str()));
		}
		(*it).active = false;
	}
	std::cout << "Ending chase!" << std::endl;
	this->running = false;
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
	std::string value;
	mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n");

	if(CivetServer::getParam(conn, "chosen", value))
	{
	   std::stringstream ss;
	   std::string::size_type pos = value.find('::');
	   std::string action = value.substr(0,pos);

	   if (action.compare("Aan/Uit") == 0)
	    {
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
 		   for (std::pair<std::string, ToggleFactory::Toggle*> element  : chase.cf.toggle->togglemap)
		   {
 			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
		if (action.compare("Geluid") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
	 	   for (std::pair<std::string, SoundFactory::Sound*> element  : chase.cf.sound->soundmap)
		   {
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->filename << "</option>";
		   }
		   ss << "</select>";
		}
		if (action.compare("Motor") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
 		   for (std::pair<std::string, MotorFactory::Motor*> element  : chase.cf.motor->motormap)
		   {
 			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
		if (action.compare("Muziek") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
     	   for (std::pair<std::string, MusicFactory::Music*> element  : chase.cf.music->musicmap)
		   {
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->filename << "</option>";
		   }
		   ss << "</select>";
		}
		if (action.compare("Scene") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
 		   for (std::pair<std::string, SceneFactory::Scene*> element  : chase.cf.scene->scenemap)
		   {
 			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
	    mg_printf(conn, ss.str().c_str());
	}
	else
	if(CivetServer::getParam(conn, "add", value))
	{
	   std::stringstream ss;
	   ss << "<html><head>";
	   ss << "<script type=\"text/javascript\" src=\"resources/jquery-3.2.0.min.js\"></script>";
	   ss << "<script type=\"text/javascript\">";
	   ss << " $(document).ready(function(){";
	   ss << "  $(\"#tijd_div\").hide();";
	   ss << "  $(\"#action\").change(function(){";
	   ss << "  if ($(\"#action\").val() == 'Tijd::Wachten') { ";
	   ss << "    $(\"#target\").hide();";
	   ss << "    $(\"#tijd_div\").show();";
	   ss << "   } else {";
	   ss << "    $(\"#target\").show();";
	   ss << "    $(\"#tijd_div\").hide();";
	   ss << "  }";
	   ss << "  action_val = $(\"#action\").val();";
	   ss << "  $.get( \"" << chase.getUrl() << "?chosen=\"+action_val, function( data ) {";
	   ss << "  $( \"#target\" ).html( data );";
	   ss << "  if ($(\"#target\").val() != '') { ";
	   ss << "    $(\"#submit\").prop('disabled',false);";
	   ss << "   } else";
	   ss << "   {$(\"#submit\").prop('disabled',true);}";
	   ss << "  });";
	   ss << " });";
	   ss << "});";
	   ss << "</script>";
	   ss << "<script type=\"text/javascript\">";
	   ss << " $(document).ready(function(){";
	   ss << "  $(\"#target\").change(function(){";
	   ss << "  if ($(\"#target\").val() != '') { ";
	   ss << "    $(\"#submit\").prop('disabled',false);";
	   ss << "   } else {$(\"#submit\").prop('disabled',true);}";
	   ss << "  });";
	   ss << " });";
	   ss << "</script>";
	   ss << "</head>";
	   ss << "<body>";
	   ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
	   ss << " <input type=\"hidden\" name=\"iter\" value=\"" << value << "\">";
	   ss << " <select id=\"action\" name=\"action\">";
	   ss << "  <option>Aan/Uit::Aan</option>";
	   ss << "  <option>Aan/Uit::Uit</option>";
	   ss << "  <option>Geluid::Play</option>";
	   ss << "  <option>Geluid::Stop</option>";
	   ss << "  <option>Motor::Links</option>";
	   ss << "  <option>Motor::Rechts</option>";
	   ss << "  <option>Motor::Stop</option>";
	   ss << "  <option>Motor::Wachten</option>";
	   ss << "  <option>Muziek::Play</option>";
	   ss << "  <option>Muziek::Stop</option>";
	   ss << "  <option>Scene::Play</option>";
	   ss << "  <option>Tijd::Wachten</option>";
	   ss << " </select>";
	   ss << " <select id=\"target\" name=\"target\">";
	   ss << "  <option></option>";
	   ss << " </select>";
	   ss << "<div id=\"tijd_div\"><label for=\"tijd\">Milliseconden (1000 = 1 seconde):</label>"
	   		  "<input id=\"tijd\" type=\"text\" size=\"10\" name=\"target\"/>" << "</br></div>";
	   ss << "<button type=\"submit\" name=\"submit_action\" value=\"submit_action\" id=\"submit\" disabled>Submit</button></br>";
	   ss << "</br>";
	   ss << "</form>";
       ss << "<a href=\"/chasefactory\">Chases</a>";
	   ss << "<br>";
	   ss << "<a href=\"/\">Home</a>";
	   ss << "</body></html>";

	   mg_printf(conn, ss.str().c_str());
	}
	else
   {
	/* if parameter submit_action is present we want to add an action */
	if(CivetServer::getParam(conn, "submit_action", dummy))
		{
		   CivetServer::getParam(conn,"action", s[0]);
		   CivetServer::getParam(conn,"target", s[1]);
		   CivetServer::getParam(conn,"iter", s[2]);

		   sequence_item sequence = {s[0],s[1]};
		   std::list<sequence_item>::iterator first = chase.sequence_list->begin();
		   if (!(s[2].compare("-1") == 0))
		   {
			   std::advance(first, atoi(s[2].c_str()) + 1);
			   chase.sequence_list->insert(first, sequence);
		   }
		   else
		   {
			   chase.sequence_list->push_front(sequence);
		   }

		   std::stringstream ss;
		   ss << "<html><head>";
		   ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
		   mg_printf(conn, ss.str().c_str());
		   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	else
	/* if parameter submit is present the submit button was pushed */
	if(CivetServer::getParam(conn, "submit", dummy))
	{
	   CivetServer::getParam(conn,"naam", s[0]);
	   chase.naam = s[0].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[1]);
	   chase.omschrijving = s[1].c_str();

	   std::stringstream ss;
	   ss << "<html><head>";
	   ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "start", dummy))
	{
	   chase.Start();
	   std::stringstream ss;
	   ss << "<html><head>";
	   ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Starten...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		chase.Stop();
		std::stringstream ss;
		ss << "<html><head>";
		ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stoppen...!</h2>");
	}
	/* if parameter delete is present delete button was pushed */
	else if(CivetServer::getParam(conn, "delete", value))
	{
		std::list<sequence_item>::iterator first = chase.sequence_list->begin();
		std::advance(first, atoi(value.c_str()));
		chase.sequence_list->erase(first);
		std::stringstream ss;
		ss << "<html><head>";
		ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Verwijderen...!</h2>");
	}
	/* if parameter up is present up button was pushed */
	else if(CivetServer::getParam(conn, "up", value))
	{
		std::list<sequence_item>::iterator first = chase.sequence_list->begin();
		std::list<sequence_item>::iterator second = chase.sequence_list->begin();
		std::advance(first, atoi(value.c_str()));
		/* if not already in first place advance */
		if (!(std::distance(first,chase.sequence_list->begin()) == 0))
		{
			std::advance(second, atoi(value.c_str())-1);
			std::swap(*first,*second);
		}
		std::stringstream ss;
		ss << "<html><head>";
		ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Verplaatsen...!</h2>");
	}
	/* if parameter down is present down button was pushed */
	else if(CivetServer::getParam(conn, "down", value))
	{
		std::list<sequence_item>::iterator first = chase.sequence_list->begin();
		std::list<sequence_item>::iterator second = chase.sequence_list->begin();
		std::advance(first, atoi(value.c_str()));
		/* if not already in last place advance (end() points to after last element */
		if (!(std::distance(first,chase.sequence_list->end()) == 1))
		{
			std::advance(second, atoi(value.c_str())+1);
			std::swap(*first,*second);
		}
		std::stringstream ss;
		ss << "<html><head>";
		ss << "<meta http-equiv=\"refresh\" content=\"1;url=\"" << chase.getUrl() << "\"/>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Verplaatsen...!</h2>");
	}
	else
	{
		std::stringstream ss;
		ss << "<html><head>";
	   	mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "<h2>&nbsp;</h2>");
	}
	/* initial page display */
	{
		std::stringstream ss;
		ss << "<style>";
		ss << "table {width:100%;}";
		ss << "tr:nth-child(even){background-color: #eee;}";
		ss << "table, th, td{border: 1px solid black;border-collapse: collapse;}";
		ss << "th, td {padding: 5px;text-align: left;}";
		ss << "</style>";
        ss << "</head><body>";
		ss << "<h2>Chases:</h2>";
		ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  chase.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  chase.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
		ss << "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">START</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
	    ss <<  "</br>";
		std::list<sequence_item>::iterator it_list;
	    ss << "<h2>Acties:</h2>";
	    ss << "<table>";
	    ss << "<tr><th><button type=\"submit\" name=\"add\" value=\"-1\" id=\"add\">&#8627;</button>&nbsp;Nieuw</th>";
	    ss << "<th>Verwijder</th><th>Omhoog</th><th>Omlaag</th><th>Actie</th><th>Waarde</th></tr>";
		for (it_list = chase.sequence_list->begin(); it_list != chase.sequence_list->end(); ++it_list)
		{
			std::string::size_type pos = (*it_list).action.find('::');
			std::string action = (*it_list).action.substr(0,pos);
			ss << "<tr>";
			ss << "<td>" << "<button type=\"submit\" name=\"add\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"add\">&#8627;</button>";
			ss << "<td>" << "<button type=\"submit\" name=\"delete\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"delete\" style=\"font-weight:bold\">&#x1f5d1;</button>";
			ss << "<td>" << "<button type=\"submit\" name=\"up\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"up\">&uarr;</button>";
			ss << "<td>" << "<button type=\"submit\" name=\"down\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"down\">&darr;</button>";

			if (action.compare("Aan/Uit") == 0)
			{
				if (chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds) == chase.cf.toggle->togglemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\">";
				   ss << (*it_list).action << "</td>";
				   ss << "<td bgcolor=\"red\">Ongeldige Verwijzing! Opslaan vergeten of verwijderd?</td>";
				}
				else
				{
				   if ((*it_list).active)
					   ss << "<td bgcolor=\"lime\">";
				   else
					   ss << "<td>";
				   ss << (*it_list).action << "</td>";
				   ss << "<td><a href=\"" << chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
				   ss << chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></td>";
				}
			}
			if (action.compare("Geluid") == 0)
			{
				if (chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds) == chase.cf.sound->soundmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\">";
				   ss << (*it_list).action << "</td>";
				   ss << "<td bgcolor=\"red\">Ongeldige Verwijzing! Opslaan vergeten of verwijderd?</td>";
				}
				else
				{
					if ((*it_list).active)
						ss << "<td bgcolor=\"lime\">";
					else
						ss << "<td>";
					ss << (*it_list).action << "</td>";
					ss << "<td><a href=\"" << chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds)->second->filename << "</a></td>";
				}
			}
			if (action.compare("Motor") == 0)
			{
				if (chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds) == chase.cf.motor->motormap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\">";
				   ss << (*it_list).action << "</td>";
				   ss << "<td bgcolor=\"red\">Ongeldige Verwijzing! Opslaan vergeten of verwijderd?</td>";
				}
				else
				{
					if ((*it_list).active)
						ss << "<td bgcolor=\"lime\">";
					else
						ss << "<td>";
					ss << (*it_list).action << "</td>";
					ss << "<td><a href=\"" << chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></td>";
			    }
			}
			if (action.compare("Muziek") == 0)
			{
				if (chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds) == chase.cf.music->musicmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\">";
				   ss << (*it_list).action << "</td>";
				   ss << "<td bgcolor=\"red\">Ongeldige Verwijzing! Opslaan vergeten of verwijderd?</td>";
				}
				else
				{
					if ((*it_list).active)
						ss << "<td bgcolor=\"lime\">";
					else
						ss << "<td>";
		    		ss << (*it_list).action << "</td>";
			    	ss << "<td><a href=\"" << chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
				    ss << chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds)->second->filename << "</a></td>";
				}
			}
			if (action.compare("Scene") == 0)
			{
				if (chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds) == chase.cf.scene->scenemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\">";
				   ss << (*it_list).action << "</td>";
				   ss << "<td bgcolor=\"red\">Ongeldige Verwijzing! Opslaan vergeten of verwijderd?</td>";
				}
				else
				{
					if ((*it_list).active)
						ss << "<td bgcolor=\"lime\">";
					else
						ss << "<td>";
					ss << (*it_list).action << "</td>";
					ss << "<td><a href=\"" << chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></td>";
				}
			}
			if (action.compare("Tijd") == 0)
			{
				if ((*it_list).active)
					ss << "<td bgcolor=\"lime\">";
				else
					ss << "<td>";
				ss << (*it_list).action << "</td>";
				ss << "<td>" << (*it_list).uuid_or_milliseconds << "</td>";
			}
			ss << "</tr>";
		}
		ss << "</table>";
		ss << "<br>";
		ss << "</form>";
		ss << "<a href=\"/chasefactory\">Chases</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
   }
	return true;
}

