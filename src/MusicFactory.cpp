/*
 * MusicFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "MusicFactory.hpp"

std::mutex m_music;

/*
 * MusicFactory Constructor en Destructor
 */
MusicFactory::MusicFactory(){
    mfh = new MusicFactory::MusicFactoryHandler(*this);
	server->addHandler("/musicfactory", mfh);
	load();
}

MusicFactory::~MusicFactory(){
	delete mfh;
	std::map<std::string, MusicFactory::Music*>::iterator it = musicmap.begin();
	if (it != musicmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    musicmap.erase(it);
	}
}

/*
 * MusicFactoryHandler Constructor en Destructor
 */
MusicFactory::MusicFactoryHandler::MusicFactoryHandler(MusicFactory& musicfactory):musicfactory(musicfactory){
}


MusicFactory::MusicFactoryHandler::~MusicFactoryHandler(){
}

/*
 * Music Constructor en Destructor
 */
MusicFactory::Music::Music(std::string fn){
	mh = new MusicFactory::Music::MusicHandler(*this);
	this->openFromFile(fn);
	uuid_generate( (unsigned char *)&uuid );
	this->filename = fn;

	std::stringstream ss;
	ss << "/music-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

MusicFactory::Music::Music(std::string uuidstr, std::string fn, bool loop, float volume, float pitch, unsigned int fadesteps){
	mh = new MusicFactory::Music::MusicHandler(*this);
	this->openFromFile(fn);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);
	this->filename = fn;

	std::stringstream ss;
	ss << "/music-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);

	setLoop(loop);
	setVolume(volume);
	setPitch(pitch);
	setFadeSteps(fadesteps);
}

MusicFactory::Music::~Music(){
	delete mh;
}

/*
 * Music Handler Constructor en Destructor
 */
MusicFactory::Music::MusicHandler::MusicHandler(MusicFactory::Music& music):music(music){
}

MusicFactory::Music::MusicHandler::~MusicHandler(){
}


/* overige functies
 *
 */

void MusicFactory::load(){
	for (std::pair<std::string, MusicFactory::Music*> element  : musicmap)
	{
		deleteMusic(element.first);
	}

	char filename[] = CONFIG_FILE_MUSIC;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_MUSIC);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string fn = node[i]["filename"].as<std::string>();
		bool loop = node[i]["loop"].as<bool>();
		float volume = node[i]["volume"].as<float>();
		float pitch = node[i]["pitch"].as<float>();
		unsigned int fadesteps = (unsigned int) node[i]["fadesteps"].as<int>();
		MusicFactory::Music * music = new MusicFactory::Music(uuidstr, fn, loop, volume, pitch, fadesteps);
		std::string uuid_str = music->getUuid();
		musicmap.insert(std::make_pair(uuid_str,music));
	}
}

void MusicFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_MUSIC);
	std::map<std::string, MusicFactory::Music*>::iterator it = musicmap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, MusicFactory::Music*> element  : musicmap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "filename";
		emitter << YAML::Value << element.second->filename;
		emitter << YAML::Key << "loop";
		emitter << YAML::Value << element.second->getLoop();
		emitter << YAML::Key << "volume";
		emitter << YAML::Value << element.second->getVolume();
		emitter << YAML::Key << "pitch";
		emitter << YAML::Value << element.second->getPitch();
		emitter << YAML::Key << "fadesteps";
		emitter << YAML::Value << element.second->getFadeSteps();
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

std::string MusicFactory::Music::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string MusicFactory::Music::getFilename(){
	return filename;
}

std::string MusicFactory::Music::getUrl(){
	return url;
}

unsigned int MusicFactory::Music::getFadeSteps(){
	return fadesteps;
}

void MusicFactory::Music::setFadeSteps(unsigned int fadesteps){
	this->fadesteps = fadesteps;
}

void MusicFactory::Music::fadeOut(){
	std::thread( [this] {
	std::unique_lock<std::mutex> l(m_music);
	float volume = this->getVolume();
	float step = volume / fadesteps;
	for (float i = volume; i > 0; i-= step)
	{
		this->setVolume(i);
		delay(20);
	}
	this->stop();
	this->setVolume(volume);
	l.unlock();
	} ).detach();
}

void MusicFactory::Music::fadeIn(){
	std::thread( [this] {
	std::unique_lock<std::mutex> l(m_music);
	float volume = this->getVolume();
	float step = volume / fadesteps;
	this->setVolume(0);
	this->play();
	for (float i = 0; i < volume; i+= step)
	{
		this->setVolume(i);
		delay(20);
	}
	this->setVolume(volume);
	l.unlock();
	} ).detach();
}

MusicFactory::Music* MusicFactory::addMusic(std::string fn){
	MusicFactory::Music * music = new MusicFactory::Music(fn);
	std::string uuid_str = music->getUuid();
	musicmap.insert(std::make_pair(uuid_str,music));
	return music;
}

void MusicFactory::deleteMusic(std::string uuid){
	std::map<std::string, MusicFactory::Music*>::iterator it = musicmap.begin();
    it = musicmap.find(uuid);
	if (it != musicmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    musicmap.erase(it);
	}
}

bool MusicFactory::MusicFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return MusicFactory::MusicFactoryHandler::handleAll("GET", server, conn);
	}

bool MusicFactory::MusicFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return MusicFactory::MusicFactoryHandler::handleAll("POST", server, conn);
	}

bool MusicFactory::Music::MusicHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return MusicFactory::Music::MusicHandler::handleAll("GET", server, conn);
	}

bool MusicFactory::Music::MusicHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return MusicFactory::Music::MusicHandler::handleAll("POST", server, conn);
	}

bool MusicFactory::MusicFactoryHandler::handleAll(const char *method,
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
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/musicfactory\" />";
	   this->musicfactory.deleteMusic(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
        meta = "<meta http-equiv=\"refresh\" content=\"1;url=/musicfactory\" />";
		message = "Opgeslagen!";
		this->musicfactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
        meta = "<meta http-equiv=\"refresh\" content=\"1;url=/musicfactory\" />";
		message = "Ingeladen!";
		this->musicfactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		MusicFactory::Music* music = musicfactory.addMusic(value);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + music->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   DIR *dirp;
	   struct dirent *dp;
	   message = "Geuploade bestanden:";
	   ss << "<form action=\"/musicfactory\" method=\"POST\">";
	   if ((dirp = opendir(MUSIC_DIR)) == NULL) {
	          fprintf(stderr,"couldn't open %s.\n",MUSIC_DIR);
	   }
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newselect\" value=\"" << MUSIC_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newselect\">Selecteren</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "<br>";
	        }
	   } while (dp != NULL);
       ss << "</form>";
       (void) closedir(dirp);
 	}
	else
	/* initial page display */
	{
		std::map<std::string, MusicFactory::Music*>::iterator it = musicfactory.musicmap.begin();
	    for (std::pair<std::string, MusicFactory::Music*> element : musicfactory.musicmap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << "Filename:&nbsp;" << element.second->getFilename();
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/musicfactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->getUuid() << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
		}
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/musicfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/musicfactory\" method=\"POST\">";
	   	ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/musicfactory\" method=\"POST\">";
	   	ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "music",  ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool MusicFactory::Music::MusicHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string value;
	std::string dummy;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	if(CivetServer::getParam(conn,"volume", value))
		music.setVolume(std::stof(value));

	if(CivetServer::getParam(conn,"pitch", value))
		music.setPitch(std::stof(value));

	if(CivetServer::getParam(conn,"fade", value))
		music.setFadeSteps(std::stof(value));

	if(CivetServer::getParam(conn, "submit", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Wijzigingen opgeslagen!";
	} else
	if(CivetServer::getParam(conn, "play", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		music.play();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Afspelen!";
	} else
	if(CivetServer::getParam(conn, "fadein", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		music.fadeIn();
		delay(20);
		std::unique_lock<std::mutex> l(m_music);
		l.unlock();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Fade in!";
	} else
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		music.stop();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Stoppen!";
	} else
	if(CivetServer::getParam(conn, "fadeout", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		music.fadeOut();
		delay(20);
		std::unique_lock<std::mutex> l(m_music);
		l.unlock();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Fade out!";
	} else
	if(CivetServer::getParam(conn, "pause", dummy))
	{
		 if(CivetServer::getParam(conn,"loop", dummy))
		  		music.setLoop(true);
		   	else
		   		music.setLoop(false);
		music.pause();
		std::stringstream ss;
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + music.getUrl() + "\"/>";
		message = "Pauze!";
	}

	{
		ss << "Bestandsnaam: " << music.filename << "<br>";
		ss << "<br>";
		ss << "<form action=\"" << music.getUrl() << "\" method=\"POST\">";
		ss << "<div class=\"container\">";
		ss << "<label for=\"volume\">Volume</label>"
			  "<input class=\"inside\" id=\"volume\" type=\"range\" min=\"0\" max=\"100\" step=\"1\" value=\"" <<
			  music.getVolume() << "\"" << " name=\"volume\"/>" << "<br>";
		ss << "<label for=\"pitch\">Pitch</label>"
			  "<input class=\"inside\" id=\"pitch\" type=\"range\" min=\"0.8\" max=\"1.2\" step=\"0.02\" value=\"" <<
			  music.getPitch() << "\"" << " name=\"pitch\"/>" << "<br>";
		ss << "<label for=\"fade\">Fade Steps</label>"
			  "<input class=\"inside\" id=\"fade\" type=\"range\" min=\"1\" max=\"200\" step=\"1\" value=\"" <<
			  music.getFadeSteps() << "\"" << " name=\"fade\"/>" << "<br>";
		ss << "<label for=\"loop\">Loop</label>";
		if (music.getLoop())
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\" checked=\"checked\">";
		else
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\">";
		ss << "</div>";
		ss << "<br>";
		ss << "<br>";
		ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button><br>";
		ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"play\" value=\"play\" id=\"play\">Play</button>";
	    ss << "<button type=\"submit\" name=\"fadein\" value=\"fadein\" id=\"fadein\">Fade In</button>";
		ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">Stop</button>";
		ss << "<button type=\"submit\" name=\"fadeout\" value=\"fadeout\" id=\"fadeout\">Fade Out</button>";
		ss << "<button type=\"submit\" name=\"pause\" value=\"pause\" id=\"pause\">Pause</button>";
		ss << "</form>";
	}

	ss = getHtml(meta, message, "music", ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}



