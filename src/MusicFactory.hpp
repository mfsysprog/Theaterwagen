/*
 * MusicFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef MUSICFACTORY_HPP_
#define MUSICFACTORY_HPP_

#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include <SFML/Audio.hpp>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>

#define RESOURCES_DIR "/home/erik/resources/"

extern CivetServer* server;

class MusicFactory {
	private:
	class MusicFactoryHandler : public CivetHandler
	{
		public:
		MusicFactoryHandler(MusicFactory& musicfactory);
		~MusicFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		MusicFactory& musicfactory;
	};
	class Music {
		public:
		Music(std::string fn);
		~Music();
		std::string getUuid();
		std::string getFilename();
		std::string getUrl();
		private:
		class MusicHandler : public CivetHandler
		{
			public:
			MusicHandler(Music& music);
			~MusicHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Music& music;
		};
		void Pause();
		void Play();
		void Stop();
		bool getLoop();
		void setLoop(bool loop);
		void setVolume(float vol);
		float getVolume();
		std::string filename;
		std::string url;
		uuid_t uuid;
		sf::Music* sfm;
	    MusicHandler* mh;
	};
	public:
	MusicFactory();
	~MusicFactory();
	MusicFactory::Music* addMusic(std::string fn);

	private:
	std::map<std::string, MusicFactory::Music*> musicmap;
	MusicFactoryHandler* mfh;
};


#endif /* MUSICFACTORY_HPP_ */
