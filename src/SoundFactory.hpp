/*
 * SoundFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef SOUNDFACTORY_HPP_
#define SOUNDFACTORY_HPP_

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

class SoundFactory {
	private:
	class SoundFactoryHandler : public CivetHandler
	{
		public:
		SoundFactoryHandler(SoundFactory& soundfactory);
		~SoundFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		SoundFactory& soundfactory;
	};
	class Sound {
		public:
		Sound(std::string fn);
		~Sound();
		std::string getUuid();
		std::string getFilename();
		std::string getUrl();
		private:
		class SoundHandler : public CivetHandler
		{
			public:
			SoundHandler(Sound& sound);
			~SoundHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Sound& sound;
		};
		void Pause();
		void Play();
		void Stop();
		bool getLoop();
		void setLoop(bool loop);
		void setVolume(float vol);
		float getVolume();
		void setPitch(float vol);
		float getPitch();
		std::string filename;
		std::string url;
		uuid_t uuid;
		sf::Sound* sfm;
		sf::SoundBuffer* sfmbuffer;
	    SoundHandler* mh;
	};
	public:
	SoundFactory();
	~SoundFactory();
	SoundFactory::Sound* addSound(std::string fn);
	void deleteSound(std::string uuid);

	private:
	std::map<std::string, SoundFactory::Sound*> soundmap;
	SoundFactoryHandler* mfh;
};


#endif /* SOUNDFACTORY_HPP_ */
