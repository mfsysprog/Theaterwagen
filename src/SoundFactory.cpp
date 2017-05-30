/*
 * SoundFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "SoundFactory.hpp"

std::mutex m_sound;

/*
 * SoundFactory Constructor en Destructor
 */
SoundFactory::SoundFactory(){
    mfh = new SoundFactory::SoundFactoryHandler(*this);
	server->addHandler("/soundfactory", mfh);
	load();
}

SoundFactory::~SoundFactory(){
	delete mfh;
	std::map<std::string, SoundFactory::Sound*>::iterator it = soundmap.begin();
	if (it != soundmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    soundmap.erase(it);
	}
}

/*
 * SoundFactoryHandler Constructor en Destructor
 */
SoundFactory::SoundFactoryHandler::SoundFactoryHandler(SoundFactory& soundfactory):soundfactory(soundfactory){
}


SoundFactory::SoundFactoryHandler::~SoundFactoryHandler(){
}

/*
 * Sound Constructor en Destructor
 */
SoundFactory::Sound::Sound(std::string fn){
	mh = new SoundFactory::Sound::SoundHandler(*this);
	sfmbuffer = new sf::SoundBuffer();
	sfmbuffer->loadFromFile(fn);
	this->setBuffer(*sfmbuffer);
	uuid_generate( (unsigned char *)&uuid );
	this->filename = fn;

	std::stringstream ss;
	ss << "/sound-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

SoundFactory::Sound::Sound(std::string uuidstr, std::string fn, bool loop, float volume, float pitch, unsigned int fadesteps){
	mh = new SoundFactory::Sound::SoundHandler(*this);
	sfmbuffer = new sf::SoundBuffer();
	sfmbuffer->loadFromFile(fn);
	this->setBuffer(*sfmbuffer);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);
	this->filename = fn;

	std::stringstream ss;
	ss << "/sound-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);

	this->setLoop(loop);
	this->setVolume(volume);
	this->setPitch(pitch);
	setFadeSteps(fadesteps);
}

SoundFactory::Sound::~Sound(){
	delete mh;
	delete sfmbuffer;
}

/*
 * Sound Handler Constructor en Destructor
 */
SoundFactory::Sound::SoundHandler::SoundHandler(SoundFactory::Sound& sound):sound(sound){
}

SoundFactory::Sound::SoundHandler::~SoundHandler(){
}


/* overige functies
 *
 */

void SoundFactory::load(){
	for (std::pair<std::string, SoundFactory::Sound*> element  : soundmap)
	{
		deleteSound(element.first);
	}

	char filename[] = CONFIG_FILE_SOUND;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_SOUND);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string fn = node[i]["filename"].as<std::string>();
		bool loop = node[i]["loop"].as<bool>();
		float volume = node[i]["volume"].as<float>();
		float pitch = node[i]["pitch"].as<float>();
		unsigned int fadesteps = (unsigned int) node[i]["fadesteps"].as<int>();
		SoundFactory::Sound * sound = new SoundFactory::Sound(uuidstr, fn, loop, volume, pitch, fadesteps);
		std::string uuid_str = sound->getUuid();
		soundmap.insert(std::make_pair(uuid_str,sound));
	}
}

void SoundFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_SOUND);
	std::map<std::string, SoundFactory::Sound*>::iterator it = soundmap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, SoundFactory::Sound*> element  : soundmap)
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


std::string SoundFactory::Sound::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string SoundFactory::Sound::getFilename(){
	return filename;
}

std::string SoundFactory::Sound::getUrl(){
	return url;
}

unsigned int SoundFactory::Sound::getFadeSteps(){
	return fadesteps;
}

void SoundFactory::Sound::setFadeSteps(unsigned int fadesteps){
	this->fadesteps = fadesteps;
}

void SoundFactory::Sound::fadeOut(){
	std::thread( [this] {
	std::unique_lock<std::mutex> l(m_sound);
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

void SoundFactory::Sound::fadeIn(){
	std::thread( [this] {
	std::unique_lock<std::mutex> l(m_sound);
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


SoundFactory::Sound* SoundFactory::addSound(std::string fn){
	SoundFactory::Sound * sound = new SoundFactory::Sound(fn);
	std::string uuid_str = sound->getUuid();
	soundmap.insert(std::make_pair(uuid_str,sound));
	return sound;
}

void SoundFactory::deleteSound(std::string uuid){
	std::map<std::string, SoundFactory::Sound*>::iterator it = soundmap.begin();
    it = soundmap.find(uuid);
	if (it != soundmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    soundmap.erase(it);
	}
}

bool SoundFactory::SoundFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return SoundFactory::SoundFactoryHandler::handleAll("GET", server, conn);
	}

bool SoundFactory::SoundFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return SoundFactory::SoundFactoryHandler::handleAll("POST", server, conn);
	}

bool SoundFactory::Sound::SoundHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return SoundFactory::Sound::SoundHandler::handleAll("GET", server, conn);
	}

bool SoundFactory::Sound::SoundHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return SoundFactory::Sound::SoundHandler::handleAll("POST", server, conn);
	}

bool SoundFactory::SoundFactoryHandler::handleAll(const char *method,
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
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/soundfactory\">";
	   this->soundfactory.deleteSound(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/soundfactory\">";
		message = "Opgeslagen!";
		this->soundfactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/soundfactory\"/>";
		message = "Ingeladen!";
		this->soundfactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		SoundFactory::Sound* sound = soundfactory.addSound(value);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + sound->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   DIR *dirp;
	   struct dirent *dp;
	   message = "Geuploade bestanden:";
	   ss << "<form action=\"/soundfactory\" method=\"POST\">";
	   if ((dirp = opendir(RESOURCES_DIR)) == NULL) {
	          fprintf(stderr,"couldn't open %s.\n",RESOURCES_DIR);
	   }
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newselect\" value=\"" << RESOURCES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newselect\">Selecteren</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "</br>";
	        }
	   } while (dp != NULL);
       ss << "</form>";
       (void) closedir(dirp);
	}
	else
	/* initial page display */
	{
		std::map<std::string, SoundFactory::Sound*>::iterator it = soundfactory.soundmap.begin();
	    for (std::pair<std::string, SoundFactory::Sound*> element : soundfactory.soundmap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << "Filename:&nbsp;" << element.second->getFilename();
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/soundfactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->getUuid() << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/soundfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/soundfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/soundfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "sound",  ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool SoundFactory::Sound::SoundHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string value;
	std::string dummy;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	if(CivetServer::getParam(conn,"volume", value))
		sound.setVolume(std::stof(value));

	if(CivetServer::getParam(conn,"pitch", value))
		sound.setPitch(std::stof(value));

	if(CivetServer::getParam(conn,"fade", value))
		sound.setFadeSteps(std::stof(value));

	if(CivetServer::getParam(conn, "submit", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Wijzigingen opgeslagen!";
	} else
	if(CivetServer::getParam(conn, "play", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		sound.play();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Afspelen!";
	} else
	if(CivetServer::getParam(conn, "fadein", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		sound.fadeIn();
		delay(20);
		std::unique_lock<std::mutex> l(m_sound);
		l.unlock();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Fade in!";
	} else
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		sound.stop();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Stoppen!";
	} else
	if(CivetServer::getParam(conn, "fadeout", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		sound.fadeOut();
		delay(20);
		std::unique_lock<std::mutex> l(m_sound);
		l.unlock();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Fade out!";
	} else
	if(CivetServer::getParam(conn, "pause", dummy))
	{
		if(CivetServer::getParam(conn,"loop", dummy))
		  		sound.setLoop(true);
		   	else
		   		sound.setLoop(false);
		sound.pause();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + sound.getUrl() + "\"/>";
		message = "Pauze!";
	}

	{
		ss << "Bestandsnaam: " << sound.filename << "</br>";
		ss << "</br>";
		ss << "<form action=\"" << sound.getUrl() << "\" method=\"POST\">";
		ss << "<div class=\"container\">";
		ss << "<label for=\"volume\">Volume</label>"
			  "<input class=\"inside\" id=\"volume\" type=\"range\" min=\"0\" max=\"100\" step=\"1\" value=\"" <<
			  sound.getVolume() << "\"" << " name=\"volume\"/>" << "</br>";
		ss << "<label for=\"pitch\">Pitch</label>"
			  "<input class=\"inside\" id=\"pitch\" type=\"range\" min=\"0.8\" max=\"1.2\" step=\"0.02\" value=\"" <<
			  sound.getPitch() << "\"" << " name=\"pitch\"/>" << "</br>";
		ss << "<label for=\"fade\">Fade Steps</label>"
			  "<input class=\"inside\" id=\"fade\" type=\"range\" min=\"1\" max=\"200\" step=\"1\" value=\"" <<
			  sound.getFadeSteps() << "\"" << " name=\"fade\"/>" << "<br>";
		ss << "<label for=\"loop\">Loop</label>";
		if (sound.getLoop())
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\" checked=\"checked\">";
		else
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\">";
		ss << "</div>";
		ss << "</br>";
		ss << "</br>";
		ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
		ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"play\" value=\"play\" id=\"play\">Play</button>";
	    ss << "<button type=\"submit\" name=\"fadein\" value=\"fadein\" id=\"fadein\">Fade In</button>";
		ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">Stop</button>";
		ss << "<button type=\"submit\" name=\"fadeout\" value=\"fadeout\" id=\"fadeout\">Fade Out</button>";
		ss << "<button type=\"submit\" name=\"pause\" value=\"pause\" id=\"pause\">Pause</button>";
		ss << "</form>";
	}

	ss = getHtml(meta, message, "sound", ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}



