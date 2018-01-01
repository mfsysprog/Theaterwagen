/*
x * LiftFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "SceneFactory.hpp"
#include "LiftFactory.hpp"

#define _(String) gettext (String)

extern std::mutex m_scene;

extern usb_dev_handle *handle;
extern bool uDMX_found;
extern unsigned char main_channel[512];

volatile long iping1      = 0;
volatile long ipong1      = 0;
volatile long iping2      = 0;
volatile long ipong2      = 0;

static std::function<void()> cb_echo1, cb_echo2;

static void CallBackEcho1()
{
  cb_echo1();
}

static void CallBackEcho2()
{
  cb_echo2();
}

/*
 * LiftFactory Constructor en Destructor
 */
LiftFactory::LiftFactory(){
    mfh = new LiftFactory::LiftFactoryHandler(*this);
	server->addHandler("/liftfactory", mfh);
	load();
}

LiftFactory::~LiftFactory(){
	delete mfh;
	std::map<std::string, LiftFactory::Lift*>::iterator it = liftmap.begin();
	if (it != liftmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    liftmap.erase(it);
	}
}

/*
 * LiftFactoryHandler Constructor en Destructor
 */
LiftFactory::LiftFactoryHandler::LiftFactoryHandler(LiftFactory& liftfactory):liftfactory(liftfactory){
}


LiftFactory::LiftFactoryHandler::~LiftFactoryHandler(){
}

/*
 * Lift Constructor en Destructor
 */
LiftFactory::Lift::Lift(std::string naam, std::string omschrijving, unsigned char channel, int gpio_dir, int gpio_trigger, int gpio_echo1, int gpio_echo2){
	lh = new LiftFactory::Lift::LiftHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->channel = channel;
	this->naam = naam;
	this->omschrijving = omschrijving;
	this->gpio_dir = gpio_dir;
	this->gpio_echo1 = gpio_echo1;
	this->gpio_echo2 = gpio_echo2;
	this->gpio_trigger = gpio_trigger;

	pinMode(gpio_trigger, OUTPUT);
	cb_echo1 = std::bind(&Lift::Echo1,this);
	cb_echo2 = std::bind(&Lift::Echo2,this);
	wiringPiISR(gpio_echo1, INT_EDGE_BOTH, &CallBackEcho1);
	wiringPiISR(gpio_echo2, INT_EDGE_BOTH, &CallBackEcho2);

	std::stringstream ss;
	ss << "/lift-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, lh);
}

LiftFactory::Lift::Lift(std::string uuidstr, std::string naam, std::string omschrijving, unsigned char channel, int gpio_dir, int gpio_trigger, int gpio_echo1, int gpio_echo2){
	lh = new LiftFactory::Lift::LiftHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->channel = channel;
	this->naam = naam;
	this->omschrijving = omschrijving;
	this->gpio_dir = gpio_dir;
	this->gpio_echo1 = gpio_echo1;
	this->gpio_echo2 = gpio_echo2;
	this->gpio_trigger = gpio_trigger;

	pinMode(gpio_trigger, OUTPUT);
	cb_echo1 = std::bind(&Lift::Echo1,this);
	cb_echo2 = std::bind(&Lift::Echo2,this);
	wiringPiISR(gpio_echo1, INT_EDGE_BOTH, &CallBackEcho1);
	wiringPiISR(gpio_echo2, INT_EDGE_BOTH, &CallBackEcho2);

	std::stringstream ss;
	ss << "/lift-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, lh);
}

LiftFactory::Lift::~Lift(){
	delete lh;
}

/*
 * Lift Handler Constructor en Destructor
 */
LiftFactory::Lift::LiftHandler::LiftHandler(LiftFactory::Lift& lift):lift(lift){
}

LiftFactory::Lift::LiftHandler::~LiftHandler(){
}


void LiftFactory::Lift::Up(){
	if (direction == LIFT_UP) return;
	else if (direction == LIFT_DOWN)
	  {
		this->Stop();
	  }

	this->direction = LIFT_UP;

	std::thread t1( [this] {

		if (!(this->getPosition(LIFT_UP)))
		{
			digitalWrite(gpio_dir, HIGH);

			delay(100);

			//TODO: for testing only
			main_channel[channel-2] =  255;
			main_channel[channel-1] =  255;

			std::unique_lock<std::mutex> l(m_scene);
			if (uDMX_found)
			{
				int nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
											cmd_SetChannelRange, 512, 0, (char *)main_channel, 512, 1000);
				if (nBytes < 0)
					fprintf(stderr, "USB error: %s\n", usb_strerror());
			}
			l.unlock();
			delay(20);

			while (!(this->getPosition(LIFT_UP)));
		}
		this->Stop();
	});
	t1.detach();
}

void LiftFactory::Lift::Down(){

	if (direction == LIFT_DOWN) return;
	else if (direction == LIFT_UP)
	  {
		this->Stop();
	  }

	this->direction = LIFT_DOWN;

	std::thread t1( [this] {

		if (!(this->getPosition(LIFT_DOWN)))
		{
			digitalWrite(gpio_dir, LOW);

			delay(100);

			//TODO: for testing only
			main_channel[channel-2] =  255;
			main_channel[channel-1] =  255;

			std::unique_lock<std::mutex> l(m_scene);
			if (uDMX_found)
			{
				int nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
											cmd_SetChannelRange, 512, 0, (char *)main_channel, 512, 1000);
				if (nBytes < 0)
					fprintf(stderr, "USB error: %s\n", usb_strerror());
			}
			l.unlock();
			delay(20);

			while (!(this->getPosition(LIFT_DOWN)));
		}
		this->Stop();
	});
	t1.detach();
}

void LiftFactory::Lift::Stop(){

	//TODO: for testing only
	main_channel[channel-2] =  0;
	main_channel[channel-1] =  0;

	std::unique_lock<std::mutex> l(m_scene);
	if (uDMX_found)
	{
		int nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
									cmd_SetChannelRange, 512, 0, (char *)main_channel, 512, 1000);
		if (nBytes < 0)
			fprintf(stderr, "USB error: %s\n", usb_strerror());
	}
	l.unlock();
	delay(100);

	digitalWrite(gpio_dir, LOW);
	this->direction = LIFT_STOP;
}

void LiftFactory::Lift::Wait(){
	while(!(this->direction = LIFT_STOP)) delay(200);
}

void LiftFactory::Lift::Echo1(){
	if (digitalRead(gpio_echo1) == HIGH) iping1 = micros();
	else ipong1 = micros();
}

void LiftFactory::Lift::Echo2(){
	if (digitalRead(gpio_echo2) == HIGH) iping2 = micros();
	else ipong2 = micros();
}

bool LiftFactory::Lift::getPosition(liftdir direction){

	//if stop was pressed manually we stop
	if (this->direction == LIFT_STOP) return true;

	float idistance1 = 0, adistance1[10] = {0}, gdistance1 = 0;
	float idistance2 = 0, adistance2[10] = {0}, gdistance2 = 0;

	for (int i = 0; i < 10; i++)
	{
		iping1      = 0;
		ipong1      = 0;
		iping2      = 0;
		ipong2      = 0;

		// Ensure trigger is low.
		digitalWrite(gpio_trigger, LOW);
		delayMicroseconds(2);

		// Trigger the ping.
		digitalWrite(gpio_trigger, HIGH);
		delayMicroseconds(10);
		digitalWrite(gpio_trigger, LOW);

		delay(30);
		// Convert ping duration to distance.
		//distance = (float) (pong - ping) * 0.017150;
		idistance1 = (float) (ipong1 - iping1) * 0.017150;
		gdistance1 += idistance1;
		adistance1[i] = idistance1;

		idistance2 = (float) (ipong2 - iping2) * 0.017150;
		gdistance2 += idistance2;
		adistance2[i] = idistance2;
	}

	gdistance1 /= 10;
	sort(adistance1, adistance1 + 10);
	float median1 = (adistance1[3] + adistance1[4] + adistance1[5] + adistance1[6]) / 4.0;

	gdistance2 /= 10;
	sort(adistance2, adistance2 + 10);
	float median2 = (adistance2[3] + adistance2[4] + adistance2[5] + adistance2[6]) / 4.0;

	//printf("Distance : %.2f cm.\n", distance);
	printf("ADistance1: %.1f cm. GDistance1: %.1f cm. ADistance2: %.1f cm. GDistance2: %.1f cm.\n", median1, gdistance1, median2, gdistance2);

	//TODO: for testing only
	if (direction == LIFT_UP && median1 > 10.0) return false;
	else if (direction == LIFT_DOWN && median1 < 70.0) return false;
	else return true;
}

/* overige functies
 *
 */

void LiftFactory::load(){
	for (std::pair<std::string, LiftFactory::Lift*> element  : liftmap)
	{
		deleteLift(element.first);
	}

	char filename[] = CONFIG_FILE_LIFT;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_LIFT);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		unsigned char channel = atoi(node[i]["channel"].as<std::string>().c_str());
		int gpio_dir = node[i]["gpio_dir"].as<int>();
		int gpio_trigger = node[i]["gpio_trigger"].as<int>();
		int gpio_echo1 = node[i]["gpio_echo1"].as<int>();
		int gpio_echo2 = node[i]["gpio_echo2"].as<int>();

		LiftFactory::Lift * lift = new LiftFactory::Lift(uuidstr, naam, omschrijving, channel, gpio_dir, gpio_trigger, gpio_echo1, gpio_echo2);
		std::string uuid_str = lift->getUuid();
		liftmap.insert(std::make_pair(uuid_str,lift));
	}
}

void LiftFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_LIFT);
	std::map<std::string, LiftFactory::Lift*>::iterator it = liftmap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, LiftFactory::Lift*> element  : liftmap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "channel";
		emitter << std::to_string(element.second->channel);
		emitter << YAML::Key << "gpio_dir";
		emitter << YAML::Value << element.second->gpio_dir;
		emitter << YAML::Key << "gpio_trigger";
		emitter << YAML::Value << element.second->gpio_trigger;
		emitter << YAML::Key << "gpio_echo1";
		emitter << YAML::Value << element.second->gpio_echo1;
		emitter << YAML::Key << "gpio_echo2";
		emitter << YAML::Value << element.second->gpio_echo2;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}


std::string LiftFactory::Lift::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string LiftFactory::Lift::getUrl(){
	return url;
}

LiftFactory::Lift* LiftFactory::addLift(std::string naam, std::string omschrijving, unsigned char channel, int gpio_dir, int gpio_trigger, int gpio_echo1, int gpio_echo2){
	LiftFactory::Lift * lift = new LiftFactory::Lift(naam, omschrijving, channel, gpio_dir, gpio_trigger, gpio_echo1, gpio_echo2);
	std::string uuid_str = lift->getUuid();
	liftmap.insert(std::make_pair(uuid_str,lift));
	return lift;
}

void LiftFactory::deleteLift(std::string uuid){
	std::map<std::string, LiftFactory::Lift*>::iterator it = liftmap.begin();
    it = liftmap.find(uuid);
	if (it != liftmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    liftmap.erase(it);
	}
}

std::string LiftFactory::Lift::getNaam(){
	return naam;
}

std::string LiftFactory::Lift::getOmschrijving(){
	return omschrijving;
}

bool LiftFactory::LiftFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return LiftFactory::LiftFactoryHandler::handleAll("GET", server, conn);
	}

bool LiftFactory::LiftFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return LiftFactory::LiftFactoryHandler::handleAll("POST", server, conn);
	}

bool LiftFactory::Lift::LiftHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return LiftFactory::Lift::LiftHandler::handleAll("GET", server, conn);
	}

bool LiftFactory::Lift::LiftHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return LiftFactory::Lift::LiftHandler::handleAll("POST", server, conn);
	}

bool LiftFactory::LiftFactoryHandler::handleAll(const char *method,
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
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/liftfactory\">";
	   this->liftfactory.deleteLift(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/liftfactory\">";
		   message = _("Saved!");
        this->liftfactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/liftfactory\">";
		   message = _("Loaded!");
		this->liftfactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		std::string uuid = value;
		LiftFactory::Lift* lift;

		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "channel", value);
		unsigned char channel = atoi(value.c_str());
		CivetServer::getParam(conn, "gpio_dir", value);
		int gpio_dir = atoi(value.c_str());
		CivetServer::getParam(conn, "gpio_trigger", value);
		int gpio_trigger = atoi(value.c_str());
		CivetServer::getParam(conn, "gpio_echo1", value);
		int gpio_echo1 = atoi(value.c_str());
		CivetServer::getParam(conn, "gpio_echo2", value);
		int gpio_echo2 = atoi(value.c_str());

		lift = liftfactory.addLift(naam, omschrijving, channel, gpio_dir, gpio_trigger, gpio_echo1, gpio_echo2);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + lift->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", value))
	{
	   ss << "<form action=\"/liftfactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << ":</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"channel\">" << _("Channel") << " (DMX) :</label>"
	   	     "<input class=\"inside\" id=\"channel\" type=\"number\" min=\"1\" max=\"512\" placeholder=\"1\" step=\"1\" name=\"channel\"/>" << "</br>";
	   ss << "<label for=\"gpio_dir\">" << _("Direction") << " GPIO:</label>"
	   	     "<input class=\"inside\" id=\"gpio_dir\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" name=\"gpio_dir\"/>" << "</br>";
	   ss << "<label for=\"gpio_trigger\">" << _("Trigger") << " GPIO:</label>"
	   	     "<input class=\"inside\" id=\"gpio_trigger\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" name=\"gpio_trigger\"/>" << "</br>";
	   ss << "<label for=\"gpio_echo1\">" << _("Echo") << " 1 GPIO:</label>"
	   	     "<input class=\"inside\" id=\"gpio_echo1\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" name=\"gpio_echo1\"/>" << "</br>";
	   ss << "<label for=\"gpio_echo2\">" << _("Echo") << " 2 GPIO:</label>"
	   	     "<input class=\"inside\" id=\"gpio_echo2\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" name=\"gpio_echo2\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"" << value << "\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";
	}
	else
	/* initial page display */
	{
		std::map<std::string, LiftFactory::Lift*>::iterator it = liftfactory.liftmap.begin();
		for (std::pair<std::string, LiftFactory::Lift*> element : liftfactory.liftmap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << "&nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving();
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/liftfactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->getUuid() << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/liftfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" value=\"-1\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/liftfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/liftfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "lift",  ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool LiftFactory::Lift::LiftHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string value;
	std::string dummy;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.naam = value.c_str();
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
		lift.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "channel", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.channel = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "gpio_dir", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.gpio_dir = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "gpio_trigger", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.gpio_trigger = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "gpio_echo1", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.gpio_echo1 = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "gpio_echo2", value))
	{
		CivetServer::getParam(conn,"value", value);
		lift.gpio_echo2 = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "up", dummy))
	{
		lift.Up();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + lift.getUrl() + "\"/>";
		message = _("Moving Up!");
	}
	if(CivetServer::getParam(conn, "down", dummy))
	{
		lift.Down();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + lift.getUrl() + "\"/>";
		message = _("Moving Down!");
	}
	if(CivetServer::getParam(conn, "stop", dummy))
	{
		lift.Stop();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + lift.getUrl() + "\"/>";
		message = _("Stopping!");
	}

	/* initial page display */
	{
		ss << "<form action=\"" << lift.getUrl() << "\" method=\"POST\">";
		ss << "<button type=\"submit\" name=\"refresh\" value=\"true\" id=\"refresh\">" << _("Refresh") << "</button><br>";
		ss << "<br>";
		ss << "<button type=\"submit\" name=\"up\" value=\"up\" id=\"up\">" << _("Move Up") << "</button>";
		ss << "<button type=\"submit\" name=\"down\" value=\"down\" id=\"down\">" << _("Move Down") << "</button>";
		ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">" << _("Stop") << "</button>";
	    ss << "</form>";
	    ss << "<div style=\"clear:both\">";
	    ss << "<h2>";
	    ss << _("Current State") << ":<br>";
	    switch(lift.direction)
	    {
	    	case LIFT_UP:
	    		ss << _("Moving Up") << "<br>";
	    		break;
	    	case LIFT_DOWN:
	    		ss << _("Moving Down") << "<br>";
	    		break;
	    	case LIFT_STOP:
	    		ss << _("Stopped") << "<br>";
	    		break;
	    }
	    ss << "</h2>";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" value=\"" <<
					  lift.getNaam() << "\"" << " name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" value=\"" <<
					  lift.getOmschrijving() << "\"" << " name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
		ss << "<label for=\"channel\">" << _("Channel") << " (DMX):</label>"
			  "<input class=\"inside\" id=\"channel\" type=\"number\" min=\"1\" max=\"512\" placeholder=\"1\" step=\"1\" value=\"" <<
			  std::to_string(lift.channel).c_str() << "\" name=\"channel\"/>" << "</br>";
		ss << "<br>";
		ss << "<label for=\"gpio_dir\">" << _("Direction") << " GPIO:</label>"
			  "<input class=\"inside\" id=\"gpio_dir\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" value=\"" <<
			  lift.gpio_dir << "\" name=\"gpio_dir\"/>" << "</br>";
		ss << "<br>";
		ss << "<label for=\"gpio_trigger\">" << _("Trigger") << " GPIO:</label>"
			  "<input class=\"inside\" id=\"gpio_trigger\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" value=\"" <<
			  lift.gpio_trigger << "\" name=\"gpio_trigger\"/>" << "</br>";
		ss << "<label for=\"gpio_echo1\">" << _("Echo") << " 1 GPIO:</label>"
			  "<input class=\"inside\" id=\"gpio_echo1\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" value=\"" <<
			  lift.gpio_echo1 << "\" name=\"gpio_echo1\"/>" << "</br>";
		ss << "<br>";
		ss << "<label for=\"gpio_echo2\">" << _("Echo") << " 2 GPIO:</label>"
			  "<input class=\"inside\" id=\"gpio_echo2\" type=\"number\" min=\"0\" max=\"40\" placeholder=\"0\" step=\"1\" value=\"" <<
			  lift.gpio_echo2 << "\" name=\"gpio_echo2\"/>" << "</br>";
		ss << "</tr>";
		ss << "</div>";
		ss << "<br>";
	    ss << "<br>";
	    ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#channel').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { channel: 'true', value: $('#channel').val() }, function( data ) {";
		tohead << "  $( \"#channel\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#gpio_dir').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { gpio_dir: 'true', value: $('#gpio_dir').val() }, function( data ) {";
		tohead << "  $( \"#gpio_dir\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#gpio_trigger').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { gpio_trigger: 'true', value: $('#gpio_trigger').val() }, function( data ) {";
		tohead << "  $( \"#gpio_trigger\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#gpio_echo1').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { gpio_echo1: 'true', value: $('#gpio_echo1').val() }, function( data ) {";
		tohead << "  $( \"#gpio_echo1\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#gpio_echo2').on('change', function() {";
		tohead << " $.get( \"" << lift.getUrl() << "\", { gpio_echo2: 'true', value: $('#gpio_echo2').val() }, function( data ) {";
		tohead << "  $( \"#gpio_echo2\" ).html( data );})";
	    tohead << "});";
	    tohead << "});";
		tohead << "</script>";
	}

	ss = getHtml(meta, message, "lift", ss.str().c_str(), tohead.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}


