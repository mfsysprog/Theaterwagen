/*
 * MusicFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "MusicFactory.hpp"

/*
 * MusicFactory Constructor en Destructor
 */
MusicFactory::MusicFactory(){
    mfh = new MusicFactory::MusicFactoryHandler(*this);
	server->addHandler("/musicfactory", mfh);
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
	sfm = new sf::Music();
	sfm->openFromFile(fn);
	uuid_generate( (unsigned char *)&uuid );
	this->filename = fn;

	std::stringstream ss;
	ss << "/music-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}


MusicFactory::Music::~Music(){
	delete mh;
	delete sfm;
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

void MusicFactory::Music::Pause(){
	sfm->pause();
}

void MusicFactory::Music::Play(){
	sfm->play();
}

void MusicFactory::Music::Stop(){
	sfm->stop();
}

void MusicFactory::Music::setVolume(float vol){
	sfm->setVolume(vol);
}

float MusicFactory::Music::getVolume(){
	return sfm->getVolume();
}

void MusicFactory::Music::setLoop(bool loop){
	sfm->setLoop(loop);
}

bool MusicFactory::Music::getLoop(){
	return sfm->getLoop();
}

MusicFactory::Music* MusicFactory::addMusic(std::string fn){
	MusicFactory::Music * music = new MusicFactory::Music(fn);
	std::string uuid_str = music->getUuid();
	musicmap.insert(std::make_pair(uuid_str,music));
	return music;
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

 	/* if parameter submit is present the submit button was pushed */
	if(CivetServer::getParam(conn, "submit", dummy))
	{
	   mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/musicfactory\" /></head><body>");
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	   mg_printf(conn, "</body></html>");
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		MusicFactory::Music* music = musicfactory.addMusic(value);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << music->getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	else if(CivetServer::getParam(conn, "new", dummy))
	{
       mg_printf(conn,
		        "HTTP/1.1 200 OK\r\nContent-Type: "
		         "text/html\r\nConnection: close\r\n\r\n");
       mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	   DIR *dirp;
	   struct dirent *dp;
	   std::stringstream ss;
	   ss << "<h2>Geuploade bestanden:</h2>";
	   ss << "<form action=\"/musicfactory\" method=\"POST\">";
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
       ss <<  "</br>";
       ss << "<a href=\"/musicfactory\">Muziek</a>";
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
		std::map<std::string, MusicFactory::Music*>::iterator it = musicfactory.musicmap.begin();
		ss << "<h2>Beschikbare muziek:</h2>";
	    for (std::pair<std::string, MusicFactory::Music*> element : musicfactory.musicmap) {
	    	ss << "<form action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
			ss << "Filename:&nbsp;" << element.second->getFilename() << "</br>";
			ss << "</form>";
		}
	    ss << "<form action=\"/musicfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss <<  "</br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool MusicFactory::Music::MusicHandler::handleAll(const char *method,
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
		CivetServer::getParam(conn,"volume", value);
			music.setVolume(std::stof(value));

	    if(CivetServer::getParam(conn,"loop", dummy))
	    {
     		music.setLoop(true);
	    }
     	else
     		music.setLoop(false);

		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << music.getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	if(CivetServer::getParam(conn, "play", dummy))
	{
		music.Play();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << music.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Playing...!</h2>");
	}
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		music.Stop();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << music.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stopping...!</h2>");
	}
	if(CivetServer::getParam(conn, "pause", dummy))
	{
		music.Pause();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << music.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Pausing...!</h2>");
	}
	{
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
		std::stringstream ss;
		ss << "<h2>Muziek:</h2>";
		ss << music.filename << "</br>";
		ss << "</br>";
		ss << "<form action=\"" << music.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"volume\">Volume</label>"
			  "<input id=\"volume\" type=\"range\" min=\"0\" max=\"100\" step=\"1\" value=\"" <<
			  music.getVolume() << "\"" << " name=\"volume\"/>" << "</br>";
		ss << "<label for=\"loop\">Loop</label>";
		if (music.getLoop())
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\" checked=\"checked\"\>";
		else
			ss << "<input type=\"checkbox\" name=\"loop\" id=\"loop\"\>";
		ss << "</br>";
		ss << "</br>";
		ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
		ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"play\" value=\"play\" id=\"play\">Play</button>";
		ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">Stop</button>";
		ss << "<button type=\"submit\" name=\"pause\" value=\"pause\" id=\"pause\">Pause</button>";
		ss << "</form>";
		ss <<  "</br>";
		ss << "<a href=\"/musicfactory\">Muziek</a>";
		ss <<  "</br>";
		ss << "<a href=\"/\">Home</a>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	return true;
}



