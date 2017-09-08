/*
 * ButtonFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "ButtonFactory.hpp"
#include <libintl.h>
#define _(String) gettext (String)

static std::function<void()> cbfunc_button[40];

static int getPosition(int gpio){
		return gpio - 1;
}

static void myCallback1()
{
  cbfunc_button[getPosition(1)]();
}

static void myCallback2()
{
  cbfunc_button[getPosition(2)]();
}

static void myCallback3()
{
  cbfunc_button[getPosition(3)]();
}

static void myCallback4()
{
  cbfunc_button[getPosition(4)]();
}

static void myCallback5()
{
  cbfunc_button[getPosition(5)]();
}

static void myCallback6()
{
  cbfunc_button[getPosition(6)]();
}

static void myCallback7()
{
  cbfunc_button[getPosition(7)]();
}

static void myCallback8()
{
  cbfunc_button[getPosition(8)]();
}

static void myCallback9()
{
  cbfunc_button[getPosition(9)]();
}

static void myCallback10()
{
  cbfunc_button[getPosition(10)]();
}

static void myCallback11()
{
  cbfunc_button[getPosition(11)]();
}

static void myCallback12()
{
  cbfunc_button[getPosition(12)]();
}

static void myCallback13()
{
  cbfunc_button[getPosition(13)]();
}

static void myCallback14()
{
  cbfunc_button[getPosition(14)]();
}

static void myCallback15()
{
  cbfunc_button[getPosition(15)]();
}

static void myCallback16()
{
  cbfunc_button[getPosition(16)]();
}

static void myCallback17()
{
  cbfunc_button[getPosition(17)]();
}

static void myCallback18()
{
  cbfunc_button[getPosition(18)]();
}

static void myCallback19()
{
  cbfunc_button[getPosition(19)]();
}

static void myCallback20()
{
  cbfunc_button[getPosition(20)]();
}

static void myCallback21()
{
  cbfunc_button[getPosition(21)]();
}

static void myCallback22()
{
  cbfunc_button[getPosition(22)]();
}

static void myCallback23()
{
  cbfunc_button[getPosition(23)]();
}

static void myCallback24()
{
  cbfunc_button[getPosition(24)]();
}

static void myCallback25()
{
  cbfunc_button[getPosition(25)]();
}

static void myCallback26()
{
  cbfunc_button[getPosition(26)]();
}

static void myCallback27()
{
  cbfunc_button[getPosition(27)]();
}

static void myCallback28()
{
  cbfunc_button[getPosition(28)]();
}

static void myCallback29()
{
  cbfunc_button[getPosition(29)]();
}

static void myCallback30()
{
  cbfunc_button[getPosition(30)]();
}

static void myCallback31()
{
  cbfunc_button[getPosition(31)]();
}

static void myCallback32()
{
  cbfunc_button[getPosition(32)]();
}

static void myCallback33()
{
  cbfunc_button[getPosition(33)]();
}

static void myCallback34()
{
  cbfunc_button[getPosition(34)]();
}

static void myCallback35()
{
  cbfunc_button[getPosition(35)]();
}

static void myCallback36()
{
  cbfunc_button[getPosition(36)]();
}

static void myCallback37()
{
  cbfunc_button[getPosition(37)]();
}

static void myCallback38()
{
  cbfunc_button[getPosition(38)]();
}

static void myCallback39()
{
  cbfunc_button[getPosition(39)]();
}

static void myCallback40()
{
  cbfunc_button[getPosition(40)]();
}

static fptr getCallBack(int gpio)
{
	int position = getPosition(gpio);

	switch (position)
	{
		case 0:	return &myCallback1;
		case 1:	return &myCallback2;
		case 2:	return &myCallback3;
		case 3:	return &myCallback4;
		case 4:	return &myCallback5;
		case 5:	return &myCallback6;
		case 6:	return &myCallback7;
		case 7:	return &myCallback8;
		case 8:	return &myCallback9;
		case 9:	return &myCallback10;
		case 10:return &myCallback11;
		case 11:return &myCallback12;
		case 12:return &myCallback13;
		case 13:return &myCallback14;
		case 14:return &myCallback15;
		case 15:return &myCallback16;
		case 16:return &myCallback17;
		case 17:return &myCallback18;
		case 18:return &myCallback19;
		case 19:return &myCallback20;
		case 20:return &myCallback21;
		case 21:return &myCallback22;
	    case 22:return &myCallback23;
		case 23:return &myCallback24;
		case 24:return &myCallback25;
		case 25:return &myCallback26;
		case 26:return &myCallback27;
		case 27:return &myCallback28;
		case 28:return &myCallback29;
		case 29:return &myCallback30;
		case 30:return &myCallback31;
		case 31:return &myCallback32;
		case 32:return &myCallback33;
		case 33:return &myCallback34;
		case 34:return &myCallback35;
		case 35:return &myCallback36;
		case 36:return &myCallback37;
		case 37:return &myCallback38;
		case 38:return &myCallback39;
		case 39:return &myCallback40;
	}
	return NULL;
}

/*
 * ButtonFactory Constructor en Destructor
 */
ButtonFactory::ButtonFactory(ChaseFactory& cf):cf(cf){
	mfh = new ButtonFactory::ButtonFactoryHandler(*this);
	server->addHandler("/buttonfactory", mfh);
	load();
}

ButtonFactory::~ButtonFactory(){
	delete mfh;
	std::map<std::string, ButtonFactory::Button*>::iterator it = buttonmap.begin();
	if (it != buttonmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    buttonmap.erase(it);
	}
}

/*
 * ButtonFactoryHandler Constructor en Destructor
 */
ButtonFactory::ButtonFactoryHandler::ButtonFactoryHandler(ButtonFactory& buttonfactory):buttonfactory(buttonfactory){
}


ButtonFactory::ButtonFactoryHandler::~ButtonFactoryHandler(){
}

/*
 * Button Constructor en Destructor
 */
ButtonFactory::Button::Button(ButtonFactory& bf, std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action):bf(bf){
	mh = new ButtonFactory::Button::ButtonHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->action = action;

	/*
	 * initialise gpio
	 */
	button_gpio = GPIO_button;
	led_gpio = GPIO_led;

	Initialize();

	std::stringstream ss;
	ss << "/button-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ButtonFactory::Button::Button(ButtonFactory& bf, std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action):bf(bf){
	mh = new ButtonFactory::Button::ButtonHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->action = action;

	/*
	 * initialise gpio
	 */
	button_gpio = GPIO_button;
	led_gpio = GPIO_led;

	Initialize();

	std::stringstream ss;
	ss << "/button-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

ButtonFactory::Button::~Button(){
	delete mh;
}

/*
 * Button Handler Constructor en Destructor
 */
ButtonFactory::Button::ButtonHandler::ButtonHandler(ButtonFactory::Button& button):button(button){
}

ButtonFactory::Button::ButtonHandler::~ButtonHandler(){
}


/* overige functies
 *
 */

void ButtonFactory::load(){
	for (std::pair<std::string, ButtonFactory::Button*> element  : buttonmap)
	{
		deleteButton(element.first);
	}

	char filename[] = CONFIG_FILE_BUTTON;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_BUTTON);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		int button_gpio = node[i]["button"].as<int>();
		int led_gpio = node[i]["led"].as<int>();
		std::string action = node[i]["action"].as<std::string>();
		ButtonFactory::Button * button = new ButtonFactory::Button(*this, uuidstr, naam, omschrijving, button_gpio, led_gpio, action);
		std::string uuid_str = button->getUuid();
		buttonmap.insert(std::make_pair(uuid_str,button));
	}
}

void ButtonFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_BUTTON);
	std::map<std::string, ButtonFactory::Button*>::iterator it = buttonmap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, ButtonFactory::Button*> element  : buttonmap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "button";
		emitter << YAML::Value << element.second->button_gpio;
		emitter << YAML::Key << "led";
		emitter << YAML::Value << element.second->led_gpio;
		emitter << YAML::Key << "action";
		emitter << YAML::Value << element.second->action;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void ButtonFactory::Button::Initialize(){
	/*
	 * set relay to output and full stop
	 */
	if(led_gpio > 0) pinMode(led_gpio, OUTPUT);
	if(led_gpio > 0) digitalWrite(led_gpio, HIGH);
	pinMode(button_gpio, INPUT);
	pullUpDnControl(button_gpio, PUD_UP);

	/*
	 * Initialize callback functions to Dummy
	 */
	cbfunc_button[getPosition(this->button_gpio)] = std::bind(&Button::Dummy,this);
	if ( myWiringPiISR (button_gpio, INT_EDGE_RISING) < 0 ) {
		 std::cerr << "Error setting interrupt for GPIO sensor " << std::endl;
	 }
}

std::string ButtonFactory::Button::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string ButtonFactory::Button::getUrl(){
	return url;
}

ButtonFactory::Button* ButtonFactory::addButton(std::string naam, std::string omschrijving, int GPIO_button, int GPIO_led, std::string action){
	ButtonFactory::Button * button = new ButtonFactory::Button(*this, naam, omschrijving, GPIO_button, GPIO_led, action);
	std::string uuid_str = button->getUuid();
	buttonmap.insert(std::make_pair(uuid_str,button));
	return button;
}

void ButtonFactory::deleteButton(std::string uuid){
	std::map<std::string, ButtonFactory::Button*>::iterator it = buttonmap.begin();
    it = buttonmap.find(uuid);
	if (it != buttonmap.end())
	{
	    // found it - delete it
	    delete it->second;
	    buttonmap.erase(it);
	}
}

int ButtonFactory::Button::myWiringPiISR(int val, int mask)
{
	  return wiringPiISR(val, mask, getCallBack(this->button_gpio));
}

void ButtonFactory::Button::setActive(){
	// we keep waiting until button is released before we allow it to reactivate.
	while (digitalRead(button_gpio))
	{
		pushed=true;
		delay(500);
	}
	if (led_gpio > 0) digitalWrite(led_gpio, LOW);
	pushed=false;
	cbfunc_button[getPosition(this->button_gpio)] = std::bind(&Button::Pushed,this);
	delay(500);
}

void ButtonFactory::Button::Pushed(){
	//if we received a push but button is not still pushed we probably just got
	//interference.
	if (!(digitalRead(button_gpio))) return;
	if (led_gpio > 0) digitalWrite(led_gpio, HIGH);
	try
	{
		if (!action.empty())
			bf.cf.chasemap.find(this->action)->second->Start();
	}
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        std::cout << "Cannot execute button action: " << err_msg << std::endl;
    }
    pushed = true;
	cbfunc_button[getPosition(this->button_gpio)] = std::bind(&Button::Dummy,this);
	delay(500);
}

void ButtonFactory::Button::Wait(){
	while (!(pushed))
	{
		delay(200);
	}
}

void ButtonFactory::Button::Dummy(){
}

std::string ButtonFactory::Button::getNaam(){
	return naam;
}

std::string ButtonFactory::Button::getOmschrijving(){
	return omschrijving;
}

bool ButtonFactory::ButtonFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ButtonFactory::ButtonFactoryHandler::handleAll("GET", server, conn);
	}

bool ButtonFactory::ButtonFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ButtonFactory::ButtonFactoryHandler::handleAll("POST", server, conn);
	}

bool ButtonFactory::Button::ButtonHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return ButtonFactory::Button::ButtonHandler::handleAll("GET", server, conn);
	}

bool ButtonFactory::Button::ButtonHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return ButtonFactory::Button::ButtonHandler::handleAll("POST", server, conn);
	}

bool ButtonFactory::ButtonFactoryHandler::handleAll(const char *method,
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
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/buttonfactory\">";
	   this->buttonfactory.deleteButton(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/buttonfactory\">";
	   message = _("Saved!");
       this->buttonfactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/buttonfactory\">";
		message = _("Loaded!");
		this->buttonfactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "button", value);
		int button_gpio = atoi(value.c_str());
		CivetServer::getParam(conn, "led", value);
		int led_gpio = atoi(value.c_str());
		CivetServer::getParam(conn, "action", value);
		std::string action = value;

		ButtonFactory::Button* button = buttonfactory.addButton(naam, omschrijving, button_gpio, led_gpio, action);

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + button->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/buttonfactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << ":</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"button\">" << _("Button") << " GPIO:</label>"
	   	     "<input class=\"inside\" id=\"button\" type=\"text\" size=\"3\" name=\"button\"/>" << "</br>";
	   ss << "<label for=\"led\">Led GPIO:" << _("(0 if not in use)") << ":</label>"
	   	     "<input class=\"inside\" id=\"led\" type=\"text\" size=\"3\" name=\"led\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<label for=\"action\">" << _("Action") << _("(empty if not in use)") << ":</label></br>";
	   ss << "<select id=\"action\" name=\"action\">";
	   ss << "<option value=\"\"></option>";
	 	for (std::pair<std::string, ChaseFactory::Chase*> element  : buttonfactory.cf.chasemap)
		{
	 	   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		}
       ss << "</select></br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";
   	   ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
	}
	else
	/* initial page display */
	{
		std::map<std::string, ButtonFactory::Button*>::iterator it = buttonfactory.buttonmap.begin();
		for (std::pair<std::string, ButtonFactory::Button*> element : buttonfactory.buttonmap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
			ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/buttonfactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/buttonfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/buttonfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/buttonfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "button",  ss.str().c_str());
    mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool ButtonFactory::Button::ButtonHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string param = "chan";
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;
	std::string value;
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		button.naam = value.c_str();
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
		button.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "button", value))
	{
		CivetServer::getParam(conn,"value", value);
		button.button_gpio = atoi(value.c_str());
		button.Initialize();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "led", value))
	{
		CivetServer::getParam(conn,"value", value);
		button.led_gpio = atoi(value.c_str());
		button.Initialize();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "action", dummy))
	{
		CivetServer::getParam(conn,"value", value);
		button.action = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		if (value.empty())
		{
			ss << "<option selected=\"selected\" value=\"\"></option>";
		}
		else
			ss << "<option value=\"\"></option>";
	 	for (std::pair<std::string, ChaseFactory::Chase*> element  : button.bf.cf.chasemap)
		{
	 	   if (element.first.compare(value) == 0)
	 		   ss << "<option selected=\"selected\" value=\"" << element.first << "\">" << element.second->naam << "</option>";
	 	   else
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		}
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "activate", dummy))
	{
	   button.setActive();

	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + button.getUrl() + "\"/>";
	   message = _("Button activated!");
	}

	/* initial page display */
	{
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << button.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << button.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#button').on('change', function() {";
		tohead << " $.get( \"" << button.getUrl() << "\", { button: 'true', value: $('#button').val() }, function( data ) {";
		tohead << "  $( \"#button\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#led').on('change', function() {";
		tohead << " $.get( \"" << button.getUrl() << "\", { led: 'true', value: $('#led').val() }, function( data ) {";
		tohead << "  $( \"#led\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#action').on('change', function() {";
		tohead << " $.get( \"" << button.getUrl() << "\", { action: 'true', value: $('#action').val() }, function( data ) {";
		tohead << "  $( \"#action\" ).html( data );})";
	    tohead << "});";
	    tohead << "});";
		tohead << "</script>";
		ss << "<form action=\"" << button.getUrl() << "\" method=\"POST\">";
		ss << "<button type=\"submit\" name=\"refresh\" value=\"true\" id=\"refresh\">" << _("Refresh") << "</button><br>";
		ss << "<br>";
		ss << "<button type=\"submit\" name=\"activate\" value=\"true\" id=\"activate\">" << _("Activate") << "</button>";
	    ss << "</form>";
	    ss << "<h2>";
	    ss << _("Current State") << ":<br>";
	    ss << _("Current State") << " " << _("Button") << ":&nbsp;" << digitalRead(button.button_gpio) << "<br>";
	    if (button.led_gpio > 0)
	    	ss << _("Current State") << " " << _("Led") << ":&nbsp;" << (digitalRead(button.led_gpio) ? _("1 - Button inactive") : _("0 - Button active")) << "<br>";
	    else
	    	ss << _("Current State") << " " << _("Led") << ":&nbsp;" << _("not in use.") << "<br>";
	    ss << "</h2>";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  button.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  button.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<label for=\"button\">" << _("Button") << " GPIO:</label>"
			  "<input class=\"inside\" id=\"button\" type=\"text\" size=\"4\" value=\"" <<
			  button.button_gpio << "\" name=\"button\"/>" << "</br>";
	    ss << "<label for=\"led\">Led GPIO " << _("(0 if not in use)") << ":</label>"
	          "<input class=\"inside\" id=\"led\" type=\"text\" size=\"4\" value=\"" <<
	    	  button.led_gpio << "\" name=\"led\"/>" << "</br>";
		ss << "<label for=\"action\">" << _("Action") << _("(empty if not in use)") << ":</label></br>";
	    ss << "<select id=\"action\" name=\"action\">";
		ss << "<option value=\"\"></option>";
	 	for (std::pair<std::string, ChaseFactory::Chase*> element  : button.bf.cf.chasemap)
		{
	 	   if (button.action.compare(element.first) == 0)
	 		   ss << "<option selected=\"selected\" value=\"" << element.first << "\">" << element.second->naam << "</option>";
	 	   else
	 		   ss << "<option value=\"" << element.first << "\">" << element.second->naam << "</option>";
		}
        ss << "</select></br>";
	    ss << "</div>";
	    ss << "<br>";
	    ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
	}

	ss = getHtml(meta, message, "button", ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

