/*
 * ChaseFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "ChaseFactory.hpp"
#include <libintl.h>
#define _(String) gettext (String)

/*
 * ChaseFactory Constructor en Destructor
 */
ChaseFactory::ChaseFactory(){
    mfh = new ChaseFactory::ChaseFactoryHandler(*this);
	server->addHandler("/chasefactory", mfh);
	load();

	button = new ButtonFactory(*this);
	fixture = new FixtureFactory();
	scene = new SceneFactory(fixture);
	music = new MusicFactory();
	sound = new SoundFactory();
	motor = new MotorFactory();
	toggle = new ToggleFactory();
	web = new WebHandler();
	clone = new CloneFactory();
	lift = new LiftFactory();

	std::map<std::string, ChaseFactory::Chase*>::iterator it = chasemap.begin();

	for (std::pair<std::string, ChaseFactory::Chase*> element  : chasemap)
	{
		if (element.second->autostart == true)
		{
			element.second->Start();
		}
	}

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
	delete web;
	delete clone;
	delete lift;
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
ChaseFactory::Chase::Chase(ChaseFactory& cf, std::string naam, std::string omschrijving, bool autostart):cf(cf){
	mh = new ChaseFactory::Chase::ChaseHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->autostart = autostart;
	this->running = false;

	sequence_list = new std::list<sequence_item>();

	std::stringstream ss;
	ss << "/chase-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ChaseFactory::Chase::Chase(ChaseFactory& cf, std::string uuidstr, std::string naam, std::string omschrijving, bool autostart, std::list<sequence_item>* sequence_list):cf(cf){
	mh = new ChaseFactory::Chase::ChaseHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->autostart = autostart;
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

	char filename[] = CONFIG_FILE_CHASE;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_CHASE);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		bool autostart = node[i]["autostart"].as<bool>();
		std::list<sequence_item>* sequence_list = new std::list<sequence_item>();
		for (std::size_t k=0;k<node[i]["actions"].size();k += 2) {
			sequence_item item = {node[i]["actions"][k].as<std::string>(), node[i]["actions"][k+1].as<std::string>()};
			sequence_list->push_back(item);
		}
		ChaseFactory::Chase * chase = new ChaseFactory::Chase(*this, uuidstr, naam, omschrijving, autostart, sequence_list);
		std::string uuid_str = chase->getUuid();
		chasemap.insert(std::make_pair(uuid_str,chase));
	}
}

void ChaseFactory::saveAll(){
	button->save();
	fixture->save();
	scene->save();
	music->save();
	sound->save();
	motor->save();
	toggle->save();
	clone->save();
	lift->save();
	this->save();
}

void ChaseFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_CHASE);
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
		emitter << YAML::Key << "autostart";
		emitter << YAML::Value << element.second->autostart;
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

ChaseFactory::Chase* ChaseFactory::addChase(std::string naam, std::string omschrijving, bool autostart){
	ChaseFactory::Chase * chase = new ChaseFactory::Chase(*this, naam, omschrijving, autostart);
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
	cf.clone->clearScreen();
	cf.music->shakeOff();
	std::list<sequence_item>::iterator it;
	for (it = sequence_list->begin(); it != sequence_list->end(); ++it)
	{
		if ((*it).active == true)
				(*it).active = false;
		std::string::size_type pos = (*it).action.find("::");
		std::string action = (*it).action.substr(0,pos);

		if (action.compare("Toggle") == 0)
			cf.toggle->togglemap.find((*it).uuid_or_milliseconds)->second->Stop();
		if (action.compare("Action") == 0)
			cf.chasemap.find((*it).uuid_or_milliseconds)->second->Stop();
		if (action.compare("Sound") == 0)
			cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->stop();
		if (action.compare("Motor") == 0)
			cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Stop();
		if (action.compare("Lift") == 0)
			cf.lift->liftmap.find((*it).uuid_or_milliseconds)->second->Stop();
		if (action.compare("Music") == 0 && (*it).action.find("Music::Shake") == std::string::npos)
			cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->stop();
		if (action.compare("Scene") == 0)
			cf.scene->scenemap.find((*it).uuid_or_milliseconds)->second->Stop();
	}
}

void ChaseFactory::Chase::Start(){
	if (!this->running)
	{
		std::thread( [this] { Action(); } ).detach();
	}
}

void ChaseFactory::Chase::Action()
{
	this->running = true;
	std::list<sequence_item>::iterator it;
	for (it = sequence_list->begin(); it != sequence_list->end(); ++it)
	{
		(*it).active = false;
	}

	for (it = sequence_list->begin(); it != sequence_list->end(); ++it)
	{
		if (!this->running) break;
		std::string::size_type pos = (*it).action.find("::");
		std::string action = (*it).action.substr(0,pos);
		std::string method = (*it).action.substr(pos+2,(*it).action.length()-pos-2);

		/* skippen van ongeldige verwijzigingen */
		if ((*it).invalid) continue;

		(*it).active = true;
		if (action.compare("Toggle") == 0)
		{
			if (method.compare("On") == 0)
		      cf.toggle->togglemap.find((*it).uuid_or_milliseconds)->second->Start();
			if (method.compare("Off") == 0)
		      cf.toggle->togglemap.find((*it).uuid_or_milliseconds)->second->Stop();
		}
		if (action.compare("Button") == 0)
		{
			if (method.compare("Activate") == 0)
		      cf.button->buttonmap.find((*it).uuid_or_milliseconds)->second->setActive();
			if (method.compare("Wait") == 0)
			  cf.button->buttonmap.find((*it).uuid_or_milliseconds)->second->Wait();
		}
		if (action.compare("Clone") == 0)
		{
			if (method.compare("Photo") == 0)
			  cf.clone->clonemap.find((*it).uuid_or_milliseconds)->second->captureAndMerge();
			if (method.compare("Merge") == 0)
			  cf.clone->clonemap.find((*it).uuid_or_milliseconds)->second->mergeToScreen();
			if (method.compare("toFile") == 0)
			  cf.clone->clonemap.find((*it).uuid_or_milliseconds)->second->mergeToFile();
			if (method.compare("onScreen") == 0)
			  cf.clone->clonemap.find((*it).uuid_or_milliseconds)->second->onScreen();
			if (method.compare("clearScreen") == 0)
			  cf.clone->clearScreen();
		}
		if (action.compare("Action") == 0)
		{
			if (method.compare("Parallel") == 0)
			  cf.chasemap.find((*it).uuid_or_milliseconds)->second->Start();
			if (method.compare("Play") == 0)
			  cf.chasemap.find((*it).uuid_or_milliseconds)->second->Action();
			if (method.compare("Stop") == 0)
			  cf.chasemap.find((*it).uuid_or_milliseconds)->second->Stop();
		}
		if (action.compare("Sound") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->play();
			if (method.compare("FadeIn") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->fadeIn();
			if (method.compare("Stop") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->stop();
			if (method.compare("FadeOut") == 0)
			  cf.sound->soundmap.find((*it).uuid_or_milliseconds)->second->fadeOut();
		}
		if (action.compare("Motor") == 0)
		{
			if (method.compare("Left") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Start(LEFT);
			if (method.compare("Right") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Start(RIGHT);
			if (method.compare("Stop") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Stop();
			if (method.compare("Wait") == 0)
			  cf.motor->motormap.find((*it).uuid_or_milliseconds)->second->Wait();
		}
		if (action.compare("Lift") == 0)
		{
			if (method.compare("Up") == 0)
			  cf.lift->liftmap.find((*it).uuid_or_milliseconds)->second->Up();
			if (method.compare("Down") == 0)
			  cf.lift->liftmap.find((*it).uuid_or_milliseconds)->second->Down();
			if (method.compare("Stop") == 0)
			  cf.lift->liftmap.find((*it).uuid_or_milliseconds)->second->Stop();
			if (method.compare("Wait") == 0)
			  cf.lift->liftmap.find((*it).uuid_or_milliseconds)->second->Wait();
		}
		if (action.compare("Music") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->play();
			if (method.compare("FadeIn") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->fadeIn();
			if (method.compare("Stop") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->stop();
			if (method.compare("FadeOut") == 0)
			  cf.music->musicmap.find((*it).uuid_or_milliseconds)->second->fadeOut();
			if (method.compare("ShakeOff") == 0)
			  cf.music->shakeOff();
			if (method.compare("ShakeOn") == 0)
			  cf.music->shakeOn();

		}
		if (action.compare("Portret") == 0)
		{
			if (method.compare("Before") == 0)
			  cf.web->setVoor();
			if (method.compare("After") == 0)
			  cf.web->setNa();
		}
		if (action.compare("Scene") == 0)
		{
			if (method.compare("Play") == 0)
			  cf.scene->scenemap.find((*it).uuid_or_milliseconds)->second->Play();
			if (method.compare("FadeIn") == 0)
			  cf.scene->scenemap.find((*it).uuid_or_milliseconds)->second->fadeIn();
			if (method.compare("FadeOut") == 0)
			  cf.scene->scenemap.find((*it).uuid_or_milliseconds)->second->fadeOut();
		}
		if (action.compare("Time") == 0)
		{
			delay(atoi((*it).uuid_or_milliseconds.c_str()));
		}
		(*it).active = false;
	}
	this->running = false;
}

std::string ChaseFactory::Chase::getNaam(){
	return naam;
}

std::string ChaseFactory::Chase::getOmschrijving(){
	return omschrijving;
}

bool ChaseFactory::Chase::getAutostart(){
	return autostart;
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
	std::string message="&nbsp;";
	std::string meta="";

	if(CivetServer::getParam(conn, "saveall", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/\" />";
	   message = _("All saved!");
	   std::stringstream ss;

       ss = getHtml(meta, message, "home", ss.str().c_str());
	   mg_printf(conn, ss.str().c_str(), "%s");

	   this->chasefactory.saveAll();
	   return true;
	}

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/chasefactory\" />";
	   this->chasefactory.deleteChase(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/chasefactory\" />";
	   message = _("Saved!");
	   this->chasefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/chasefactory\" />";
	   message = _("Loaded!");
       this->chasefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "autostart", value);
		bool autostart;
		if (value.compare("ja") == 0)
			autostart = true;
		else
			autostart = false;

		ChaseFactory::Chase* chase = chasefactory.addChase(naam, omschrijving, autostart);
	    meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + chase->getUrl() + "\"/>";
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/chasefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << ":</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"autostart\">" << _("Automatic Start") << ":</label>"
	         "<input id=\"autostart\" type=\"checkbox\" name=\"autostart\" value=\"ja\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>";
   	   ss << "</div>";
   	   ss << "</form>";
	}
	/* initial page display */
	else
	{
		std::map<std::string, ChaseFactory::Chase*>::iterator it = chasefactory.chasemap.begin();
	    for (std::pair<std::string, ChaseFactory::Chase*> element : chasefactory.chasemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/chasefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "chase", ss.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");

	return true;
}

bool ChaseFactory::Chase::ChaseHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = {""};
	std::string dummy;
	std::string value;
	std::string message="&nbsp";
	std::string meta="";

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		chase.naam = value.c_str();
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
		chase.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "autostart", value))
	{
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			chase.autostart = true;
		else
			chase.autostart = false;
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "running", dummy))
	{
		std::stringstream ss;
		std::list<sequence_item>::iterator it_list;
		for (it_list = chase.sequence_list->begin(); it_list != chase.sequence_list->end(); ++it_list)
		{
			ss << "<tr>";
			if ((*it_list).active)
				ss << "<td class=\"kort\" bgcolor=\"lime\"><div class=\"waarde\">&#8618;</div></td>";
    		else
    			ss << "<td class=\"kort\"><div class=\"waarde\">&nbsp;</div></td>";
			ss << "</tr>";
		}
		mg_printf(conn,  ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "chosen", value))
	{
	   std::string::size_type pos = value.find("::");
	   std::string action = value.substr(0,pos);
       std::stringstream ss;

       ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
       ss << "text/html\r\nConnection: close\r\n\r\n";

	   if (action.compare("Toggle") == 0)
	    {
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
 		   for (std::pair<std::string, ToggleFactory::Toggle*> element  : chase.cf.toggle->togglemap)
		   {
 			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
        else
    	if (action.compare("Button") == 0)
   	    {
   		   ss << "<select id=\"target\" name=\"target\">";
   		   ss << "<option value=\"\"></option>";
   		   for (std::pair<std::string, ButtonFactory::Button*> element  : chase.cf.button->buttonmap)
   		   {
  			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
   		   }
   		   ss << "</select>";
   		}
    	else
		if (action.compare("Clone") == 0)
		{
		   if (!(value.compare("Clone::clearScreen") == 0))
		   {
			   ss << "<select id=\"target\" name=\"target\">";
			   ss << "<option value=\"\"></option>";
			   for (std::pair<std::string, CloneFactory::Clone*> element  : chase.cf.clone->clonemap)
			   {
				   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
			   }
			   ss << "</select>";
		   }
		   else
		   {
     		   ss << "<select id=\"target\" name=\"target\">";
	     	   ss << "<option value=\"n.v.t.\">" << _("N/A") << "</option>";
	     	   ss << "</select>";
		   }
		}
		else
		if (action.compare("Action") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
	 	   for (std::pair<std::string, ChaseFactory::Chase*> element  : chase.cf.chasemap)
		   {
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
		else
		if (action.compare("Sound") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
	 	   for (std::pair<std::string, SoundFactory::Sound*> element  : chase.cf.sound->soundmap)
		   {
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
		else
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
		else
		if (action.compare("Lift") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
		   ss << "<option value=\"\"></option>";
 		   for (std::pair<std::string, LiftFactory::Lift*> element  : chase.cf.lift->liftmap)
		   {
 			   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		   }
		   ss << "</select>";
		}
		else
		if (action.compare("Music") == 0)
		{
			if (value.find("Music::Shake") ==  std::string::npos)
			{
				ss << "<select id=\"target\" name=\"target\">";
				ss << "<option value=\"\"></option>";
				for (std::pair<std::string, MusicFactory::Music*> element  : chase.cf.music->musicmap)
				{
					ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
				}
				ss << "</select>";
			}
			else
			{
			   ss << "<select id=\"target\" name=\"target\">";
			   ss << "<option value=\"n.v.t.\">" << _("N/A") << "</option>";
			   ss << "</select>";
			}
		}
		else
		if (action.compare("Portret") == 0)
		{
		   ss << "<select id=\"target\" name=\"target\">";
	       ss << "<option value=\"n.v.t.\">n.v.t.</option>";
           ss << "</select>";
		}
		else
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
		else
		if (action.compare("Time") == 0)
		{
		   ss << "<input id=\"tijd\" name=\"target\">";
		}
		else
		{
			   ss << "<select id=\"target\" name=\"target\">";
		       ss << "<option value=\"\"></option>";
	           ss << "</select>";
		}

	    mg_printf(conn,  ss.str().c_str(), "%s");
	    return true;
	}

	std::stringstream ss;
    std::stringstream tohead;

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

		   meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + chase.getUrl() + "\"/>";
	}

	/* if parameter start is present start button was pushed */
	if(CivetServer::getParam(conn, "start", dummy))
	{
	   chase.Start();

	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + chase.getUrl() + "\"/>";
	   message = _("Started!");
	}
	/* if parameter stop is present stop button was pushed */
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		chase.Stop();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + chase.getUrl() + "\"/>";
	   	message = _("Stopped!");
	}
	/* if parameter delete is present delete button was pushed */
	if(CivetServer::getParam(conn, "delete", value))
	{
		std::list<sequence_item>::iterator first = chase.sequence_list->begin();
		std::advance(first, atoi(value.c_str()));
		chase.sequence_list->erase(first);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + chase.getUrl() + "\"/>";
	}
	/* if parameter up is present up button was pushed */
	if(CivetServer::getParam(conn, "up", value))
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
		meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + chase.getUrl() + "\"/>";
	}
	/* if parameter down is present down button was pushed */
	if(CivetServer::getParam(conn, "down", value))
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

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + chase.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "add", value))
	{
	   tohead << "<script type=\"text/javascript\">";
	   tohead << " $(document).ready(function(){";
	   tohead << "  $(\"#tijd_div\").hide();";
	   tohead << "  $(\"#action\").change(function(){";
	   tohead << "  if ($(\"#action\").val() == 'Time::Wait') { ";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "    $(\"#tijd_div\").show();";
	   tohead << "   } else if ($(\"#action\").val() == 'Clone::clearScreen') {";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "   } else if ($(\"#action\").val() == 'Music::ShakeOff') {";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "   } else if ($(\"#action\").val() == 'Music::ShakeOn') {";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "   } else if ($(\"#action\").val() == 'Portret::Before') {";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "   } else if ($(\"#action\").val() == 'Portret::After') {";
	   tohead << "    $(\"#target\").hide();";
	   tohead << "   } else {";
	   tohead << "    $(\"#target\").show();";
	   tohead << "    $(\"#tijd_div\").hide();";
	   tohead << "  }";
	   tohead << "  action_val = $(\"#action\").val();";
	   tohead << "  $.get( \"" << chase.getUrl() << "?chosen=\"+action_val, function( data ) {";
	   tohead << "  $( \"#target\" ).html( data );";
	   tohead << "  if ($(\"#target\").val() != '') { ";
	   tohead << "    $(\"#submit\").prop('disabled',false);";
	   tohead << "   } else";
	   tohead << "   {$(\"#submit\").prop('disabled',true);}";
	   tohead << "  });";
	   tohead << " });";
	   tohead << "});";
	   tohead << "</script>";
	   tohead << "<script type=\"text/javascript\">";
	   tohead << " $(document).ready(function(){";
	   tohead << "  $(\"#target\").change(function(){";
	   tohead << "  if ($(\"#target\").val() != '') { ";
	   tohead << "    $(\"#submit\").prop('disabled',false);";
	   tohead << "   } else {$(\"#submit\").prop('disabled',true);}";
	   tohead << "  });";
	   tohead << " });";
	   tohead << "</script>";

	   ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
	   ss << " <input type=\"hidden\" name=\"iter\" value=\"" << value << "\">";
	   ss << " <select id=\"action\" name=\"action\">";
	   ss << "  <option></option>";
	   ss << "  <option value=\"Button::Activate\">" << _("Button::Activate") << "</option>";
	   ss << "  <option value=\"Button::Wait\">" << _("Button::Wait") << "</option>";
	   ss << "  <option value=\"Clone::Photo\">" << _("Clone::Photo") << "</option>";
	   ss << "  <option value=\"Clone::Merge\">" << _("Clone::Merge") << "</option>";
	   ss << "  <option value=\"Clone::toFile\">" << _("Clone::toFile") << "</option>";
	   ss << "  <option value=\"Clone::onScreen\">" << _("Clone::onScreen") << "</option>";
	   ss << "  <option value=\"Clone::clearScreen\">" << _("Clone::clearScreen") << "</option>";
	   ss << "  <option value=\"Action::Parallel\">" << _("Action::Parallel") << "</option>";
	   ss << "  <option value=\"Action::Play\">" << _("Action::Play") << "</option>";
	   ss << "  <option value=\"Action::Stop\">" << _("Action::Stop") << "</option>";
	   ss << "  <option value=\"Sound::Play\">" << _("Sound::Play") << "</option>";
	   ss << "  <option value=\"Sound::FadeIn\">" << _("Sound::FadeIn") << "</option>";
	   ss << "  <option value=\"Sound::Stop\">" << _("Sound::Stop") << "</option>";
	   ss << "  <option value=\"Sound::FadeOut\">" << _("Sound::FadeOut") << "</option>";
	   ss << "  <option value=\"Lift::Up\">" << _("Lift::Up") << "</option>";
	   ss << "  <option value=\"Lift::Down\">" << _("Lift::Down") << "</option>";
	   ss << "  <option value=\"Lift::Stop\">" << _("Lift::Stop") << "</option>";
	   ss << "  <option value=\"Lift::Wait\">" << _("Lift::Wait") << "</option>";
	   ss << "  <option value=\"Motor::Left\">" << _("Motor::Left") << "</option>";
	   ss << "  <option value=\"Motor::Right\">" << _("Motor::Right") << "</option>";
	   ss << "  <option value=\"Motor::Stop\">" << _("Motor::Stop") << "</option>";
	   ss << "  <option value=\"Motor::Wait\">" << _("Motor::Wait") << "</option>";
	   ss << "  <option value=\"Music::Play\">" << _("Music::Play") << "</option>";
	   ss << "  <option value=\"Music::FadeIn\">" << _("Music::FadeIn") << "</option>";
	   ss << "  <option value=\"Music::Stop\">" << _("Music::Stop") << "</option>";
	   ss << "  <option value=\"Music::FadeOut\">" << _("Music::FadeOut") << "</option>";
	   ss << "  <option value=\"Music::ShakeOff\">" << _("Music::ShakeOff") << "</option>";
	   ss << "  <option value=\"Music::ShakeOn\">" << _("Music::ShakeOn") << "</option>";
	   ss << "  <option value=\"Portret::Before\">" << _("Portret::Before") << "</option>";
	   ss << "  <option value=\"Portret::After\">" << _("Portret::After") << "</option>";
	   ss << "  <option value=\"Scene::Play\">" << _("Scene::Play") << "</option>";
	   ss << "  <option value=\"Scene::FadeIn\">" << _("Scene::FadeIn") << "</option>";
	   ss << "  <option value=\"Scene::FadeOut\">" << _("Scene::FadeOut") << "</option>";
	   ss << "  <option value=\"Toggle::On\">" << _("Toggle::On") << "</option>";
	   ss << "  <option value=\"Toggle::Off\">" << _("Toggle::Off") << "</option>";
	   ss << "  <option value=\"Time::Wait\">" << _("Time::Wait") << "</option>";
	   ss << " </select>";
	   ss << " <select id=\"target\" name=\"target\">";
	   ss << "  <option></option>";
	   ss << " </select>";
	   ss << "<div id=\"tijd_div\"><label for=\"tijd\">" << _("Milliseconds (1000 = 1 second)") << ":</label>"
	   		  "<input id=\"tijd\" type=\"number\" min=\"0\" placeholder=\"1000\" step=\"1\" name=\"target\"/>" << "</div><br>";
	   ss << "<button type=\"submit\" name=\"submit_action\" value=\"submit_action\" id=\"submit\" disabled>" << _("Add") << "</button></br>";
	   ss << "</br>";
	   ss << "</form>";
	}
	else
	/* initial page display */
	{
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << chase.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << chase.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
	    tohead << " $('#autostart').on('change', function() {";
   		tohead << " $.get( \"" << chase.getUrl() << "\", { autostart: 'true', value: $('#autostart').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#autostart\" ).html( data );})";
  	    tohead << "});";
		tohead << "setInterval(function(){";
		tohead << "  $.get( \"" << chase.getUrl() << "?running=true\", function( data ) {";
		tohead << "  $( \"#chase_active\" ).html( data );";
		tohead << " });},1000)";
		tohead << "});";

		tohead << "</script>";
		tohead << "<style>";
		tohead << ".wrap {display: table; width: 100%; height: 100%;}";
		tohead << ".cell-wrap {display: table-cell; vertical-align: top; height: 100%;}";
		tohead << ".cell-wrap.links {width: 10%;}";
		tohead << ".rechts tr:nth-child(even){background-color: #eee;}";
		tohead << ".waarde {height: 40px; overflow: hidden;}";
		tohead << ".kort {width:5%; text-align: center;}";
		tohead << "table {border-collapse: collapse; border-spacing: 0; height: 100%; width: 100%; table-layout: fixed;}";
		tohead << "table td, table th {border: 1px solid black;text-align: left; width:25%}";
		tohead << "</style>";
		ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">" << _("Start") << "</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">" << _("Stop") << "</button>";
	    ss << "</form>";
	    ss << "<h2>";
	    ss << "&nbsp;";
	    ss << "</h2>";
	    ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" value=\"" <<
					  chase.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" value=\"" <<
					  chase.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		if (chase.autostart)
		{
			ss << "<label for=\"autostart\">" << _("Automatic Start") << "</label>"
			   	   	  "<input id=\"autostart\" type=\"checkbox\" name=\"autostart\" value=\"ja\" checked/>" << "</br>";
		}
		else
		{
			ss << "<label for=\"autostart\">" << _("Automatic Start") << "</label>"
			   	   	  "<input id=\"autostart\" type=\"checkbox\" name=\"autostart\" value=\"ja\"/>" << "</br>";
		}
	    ss << "<br>";
	    ss << "</div>";
	    ss << "<br>";
	    ss << "<br>";
		ss << "<form action=\"" << chase.getUrl() << "\" method=\"POST\">";
	    ss << "<div class=\"wrap\">";
	    ss << "<div class=\"cell-wrap links\">";
	    ss << "<table class=\"links\">";
	    ss << "<thead><tr><th class=\"kort\"><div class=\"waarde\">" << _("Active") << "</div></th></tr></thead>";
	    ss << "<tbody id=\"chase_active\">";
		std::list<sequence_item>::iterator it_list;
	    for (it_list = chase.sequence_list->begin(); it_list != chase.sequence_list->end(); ++it_list)
	    {
	    	ss << "<tr>";
	    	if ((*it_list).active)
	    		ss << "<td class=\"kort\" bgcolor=\"lime\"><div class=\"waarde\">&#8618;</div></td>";
	    					   else
	            ss << "<td class=\"kort\"><div class=\"waarde\">&nbsp;</div></td>";
	    	ss << "</tr>";
	    }
	    ss << "</tbody>";
	    ss << "</table>";
	    ss << "</div>";
	    ss << "<div class=\"cell-wrap rechts\">";
	    ss << "<table class=\"rechts\">";
	    ss << "<thead><tr><th class=\"kort\"><div class=\"waarde\"><button type=\"submit\" name=\"add\" value=\"-1\" id=\"add\">&#8627;</button></div></th>";
	    ss << "<th class=\"kort\"><div class=\"waarde\">&nbsp;</div></th><th class=\"kort\"><div class=\"waarde\">&nbsp;</div></th><th class=\"kort\"><div class=\"waarde\">&nbsp;</div></th><th><div class=\"waarde\">" << _("Action") << "</div></th><th><div class=\"waarde\">" << _("Value") << "</div></th></tr></thead>";
		for (it_list = chase.sequence_list->begin(); it_list != chase.sequence_list->end(); ++it_list)
		{
			std::string::size_type pos = (*it_list).action.find("::");
			std::string action = (*it_list).action.substr(0,pos);
			ss << "<tr>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"add\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"add\">&#8627;</button>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"delete\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"delete\" style=\"font-weight:bold\">&#x1f5d1;</button>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"up\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"up\">&uarr;</button>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"down\" value=\"" << std::distance(chase.sequence_list->begin(), it_list) << "\" id=\"down\">&darr;</button>";

			if (action.compare("Toggle") == 0)
			{
				if (chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds) == chase.cf.toggle->togglemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
				   ss << "<td><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
				   ss << chase.cf.toggle->togglemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Button") == 0)
			{
				if (chase.cf.button->buttonmap.find((*it_list).uuid_or_milliseconds) == chase.cf.button->buttonmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
				   ss << "<td><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.button->buttonmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
				   ss << chase.cf.button->buttonmap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Clone") == 0)
			{
				if ((*it_list).action.compare("Clone::clearScreen") == 0)
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"></div></td>";
				}
				else
				if (chase.cf.clone->clonemap.find((*it_list).uuid_or_milliseconds) == chase.cf.clone->clonemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.clone->clonemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.clone->clonemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Action") == 0)
			{
				if (chase.cf.chasemap.find((*it_list).uuid_or_milliseconds) == chase.cf.chasemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.chasemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.chasemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Sound") == 0)
			{
				if (chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds) == chase.cf.sound->soundmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.sound->soundmap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Lift") == 0)
			{
				if (chase.cf.lift->liftmap.find((*it_list).uuid_or_milliseconds) == chase.cf.lift->liftmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.lift->liftmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.lift->liftmap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
			    }
			}
			if (action.compare("Motor") == 0)
			{
				if (chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds) == chase.cf.motor->motormap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.motor->motormap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
			    }
			}
			if (action.compare("Music") == 0)
			{
				if ((*it_list).action.compare("Music::ShakeOff") == 0)
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"></div></td>";
				}
				else
				if ((*it_list).action.compare("Music::ShakeOn") == 0)
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"></div></td>";
				}
				else
				if (chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds) == chase.cf.music->musicmap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
		    		ss << _((*it_list).action.c_str()) << "</div></td>";
			    	ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
				    ss << chase.cf.music->musicmap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Portret") == 0)
			{
				ss << "<td><div class=\"waarde\">";
				ss << _((*it_list).action.c_str()) << "</div></td>";
				ss << "<td><div class=\"waarde\"></div></td>";
			}
			if (action.compare("Scene") == 0)
			{
				if (chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds) == chase.cf.scene->scenemap.end())
				{
				   (*it_list).invalid = true;
				   ss << "<td bgcolor=\"red\"><div class=\"waarde\">";
				   ss << _((*it_list).action.c_str()) << "</div></td>";
				   ss << "<td bgcolor=\"red\">" << _("Invalid Action! Forgot to save or removed?") << "</div></td>";
				}
				else
				{
					ss << "<td><div class=\"waarde\">";
					ss << _((*it_list).action.c_str()) << "</div></td>";
					ss << "<td><div class=\"waarde\"><a href=\"" << chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds)->second->getUrl() << "\">";
					ss << chase.cf.scene->scenemap.find((*it_list).uuid_or_milliseconds)->second->naam << "</a></div></td>";
				}
			}
			if (action.compare("Time") == 0)
			{
				ss << "<td><div class=\"waarde\">";
				ss << _((*it_list).action.c_str()) << "</div></td>";
				ss << "<td><div class=\"waarde\">" << (*it_list).uuid_or_milliseconds << "</div></td>";
			}
			ss << "</tr>";
		}
		ss << "</table>";
		ss << "</div>";
		ss << "</div>";
		ss << "<br style=\"clear:both\">";
		ss << "</form>";
	}

	ss = getHtml(meta, message, "chase", ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

