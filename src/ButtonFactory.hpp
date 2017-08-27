/*
 * ButtonFactory.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: erik
 */

#ifndef BUTTONFACTORY_HPP_
#define BUTTONFACTORY_HPP_

#include "Theaterwagen.hpp"
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <wiringPi.h>
#include "CivetServer.h"
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <uuid/uuid.h>
#include "boost/shared_ptr.hpp"
#include "yaml-cpp/yaml.h"

class ButtonFactory;
class ChaseFactory;

#include "ChaseFactory.hpp"

#define CONFIG_FILE_BUTTON CONFIG_DIR "buttonfactory.yaml"

extern CivetServer* server;

typedef void (*fptr)();

class ButtonFactory {
	public:
	friend class ChaseFactory;
	private:
	class ButtonFactoryHandler : public CivetHandler
	{
		public:
		ButtonFactoryHandler(ButtonFactory& buttonfactory);
		~ButtonFactoryHandler();
		private:
		bool handleGet(CivetServer *server, struct mg_connection *conn);
		bool handlePost(CivetServer *server, struct mg_connection *conn);
		bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
		ButtonFactory& buttonfactory;
	};
	class Button {
	public:
		friend class ButtonFactory;
		friend class ChaseFactory;
		Button(ButtonFactory& bf,std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action);
		Button(ButtonFactory& bf,std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action);
		~Button();
		void setActive();
		std::string getUuid();
		std::string getNaam();
		std::string getOmschrijving();
		std::string getUrl();
		private:
		class ButtonHandler : public CivetHandler
		{
			public:
			ButtonHandler(Button& button);
			~ButtonHandler();
			private:
			bool handleGet(CivetServer *server, struct mg_connection *conn);
			bool handlePost(CivetServer *server, struct mg_connection *conn);
			bool handleAll(const char *method, CivetServer *server, struct mg_connection *conn);
			Button& button;
		};
		private:
		ButtonFactory& bf;
		ButtonHandler* mh;
		int myWiringPiISR(int val, int mask);
		void Initialize();
		void Pushed();
		void Dummy();
		std::string url;
		std::string naam;
		std::string omschrijving;
		uuid_t uuid;
		int button_gpio;
		int led_gpio;
		std::string action;
	};
	public:
	ButtonFactory(ChaseFactory& cf);
	~ButtonFactory();
	ButtonFactory::Button* addButton(std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action);
	void deleteButton(std::string uuid);
	void load();

	private:
	void save();
	std::map<std::string, ButtonFactory::Button*> buttonmap;
	ChaseFactory& cf;
	ButtonFactoryHandler* mfh;
};


#endif /* BUTTONFACTORY_HPP_ */
