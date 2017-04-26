/*
 * MotorFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "MotorFactory.hpp"

static std::function<void()> cbfunc_motor[80];

static int getPosition(direction dir, int gpio){
if (dir == LEFT)
		return gpio - 1;
else if (dir == RIGHT)
		return gpio + 39;
else return -1;
}

static void myCallbackLeft1()
{
  cbfunc_motor[getPosition(LEFT,1)]();
}

static void myCallbackLeft2()
{
  cbfunc_motor[getPosition(LEFT,2)]();
}

static void myCallbackLeft3()
{
  cbfunc_motor[getPosition(LEFT,3)]();
}

static void myCallbackLeft4()
{
  cbfunc_motor[getPosition(LEFT,4)]();
}

static void myCallbackLeft5()
{
  cbfunc_motor[getPosition(LEFT,5)]();
}

static void myCallbackLeft6()
{
  cbfunc_motor[getPosition(LEFT,6)]();
}

static void myCallbackLeft7()
{
  cbfunc_motor[getPosition(LEFT,7)]();
}

static void myCallbackLeft8()
{
  cbfunc_motor[getPosition(LEFT,8)]();
}

static void myCallbackLeft9()
{
  cbfunc_motor[getPosition(LEFT,9)]();
}

static void myCallbackLeft10()
{
  cbfunc_motor[getPosition(LEFT,10)]();
}

static void myCallbackLeft11()
{
  cbfunc_motor[getPosition(LEFT,11)]();
}

static void myCallbackLeft12()
{
  cbfunc_motor[getPosition(LEFT,12)]();
}

static void myCallbackLeft13()
{
  cbfunc_motor[getPosition(LEFT,13)]();
}

static void myCallbackLeft14()
{
  cbfunc_motor[getPosition(LEFT,14)]();
}

static void myCallbackLeft15()
{
  cbfunc_motor[getPosition(LEFT,15)]();
}

static void myCallbackLeft16()
{
  cbfunc_motor[getPosition(LEFT,16)]();
}

static void myCallbackLeft17()
{
  cbfunc_motor[getPosition(LEFT,17)]();
}

static void myCallbackLeft18()
{
  cbfunc_motor[getPosition(LEFT,18)]();
}

static void myCallbackLeft19()
{
  cbfunc_motor[getPosition(LEFT,19)]();
}

static void myCallbackLeft20()
{
  cbfunc_motor[getPosition(LEFT,20)]();
}

static void myCallbackLeft21()
{
  cbfunc_motor[getPosition(LEFT,21)]();
}

static void myCallbackLeft22()
{
  cbfunc_motor[getPosition(LEFT,22)]();
}

static void myCallbackLeft23()
{
  cbfunc_motor[getPosition(LEFT,23)]();
}

static void myCallbackLeft24()
{
  cbfunc_motor[getPosition(LEFT,24)]();
}

static void myCallbackLeft25()
{
  cbfunc_motor[getPosition(LEFT,25)]();
}

static void myCallbackLeft26()
{
  cbfunc_motor[getPosition(LEFT,26)]();
}

static void myCallbackLeft27()
{
  cbfunc_motor[getPosition(LEFT,27)]();
}

static void myCallbackLeft28()
{
  cbfunc_motor[getPosition(LEFT,28)]();
}

static void myCallbackLeft29()
{
  cbfunc_motor[getPosition(LEFT,29)]();
}

static void myCallbackLeft30()
{
  cbfunc_motor[getPosition(LEFT,30)]();
}

static void myCallbackLeft31()
{
  cbfunc_motor[getPosition(LEFT,31)]();
}

static void myCallbackLeft32()
{
  cbfunc_motor[getPosition(LEFT,32)]();
}

static void myCallbackLeft33()
{
  cbfunc_motor[getPosition(LEFT,33)]();
}

static void myCallbackLeft34()
{
  cbfunc_motor[getPosition(LEFT,34)]();
}

static void myCallbackLeft35()
{
  cbfunc_motor[getPosition(LEFT,35)]();
}

static void myCallbackLeft36()
{
  cbfunc_motor[getPosition(LEFT,36)]();
}

static void myCallbackLeft37()
{
  cbfunc_motor[getPosition(LEFT,37)]();
}

static void myCallbackLeft38()
{
  cbfunc_motor[getPosition(LEFT,38)]();
}

static void myCallbackLeft39()
{
  cbfunc_motor[getPosition(LEFT,39)]();
}

static void myCallbackLeft40()
{
  cbfunc_motor[getPosition(LEFT,40)]();
}

static void myCallbackRight1()
{
  cbfunc_motor[getPosition(RIGHT,1)]();
}

static void myCallbackRight2()
{
  cbfunc_motor[getPosition(RIGHT,2)]();
}

static void myCallbackRight3()
{
  cbfunc_motor[getPosition(RIGHT,3)]();
}

static void myCallbackRight4()
{
  cbfunc_motor[getPosition(RIGHT,4)]();
}

static void myCallbackRight5()
{
  cbfunc_motor[getPosition(RIGHT,5)]();
}

static void myCallbackRight6()
{
  cbfunc_motor[getPosition(RIGHT,6)]();
}

static void myCallbackRight7()
{
  cbfunc_motor[getPosition(RIGHT,7)]();
}

static void myCallbackRight8()
{
  cbfunc_motor[getPosition(RIGHT,8)]();
}

static void myCallbackRight9()
{
  cbfunc_motor[getPosition(RIGHT,9)]();
}

static void myCallbackRight10()
{
  cbfunc_motor[getPosition(RIGHT,10)]();
}

static void myCallbackRight11()
{
  cbfunc_motor[getPosition(RIGHT,11)]();
}

static void myCallbackRight12()
{
  cbfunc_motor[getPosition(RIGHT,12)]();
}

static void myCallbackRight13()
{
  cbfunc_motor[getPosition(RIGHT,13)]();
}

static void myCallbackRight14()
{
  cbfunc_motor[getPosition(RIGHT,14)]();
}

static void myCallbackRight15()
{
  cbfunc_motor[getPosition(RIGHT,15)]();
}

static void myCallbackRight16()
{
  cbfunc_motor[getPosition(RIGHT,16)]();
}

static void myCallbackRight17()
{
  cbfunc_motor[getPosition(RIGHT,17)]();
}

static void myCallbackRight18()
{
  cbfunc_motor[getPosition(RIGHT,18)]();
}

static void myCallbackRight19()
{
  cbfunc_motor[getPosition(RIGHT,19)]();
}

static void myCallbackRight20()
{
  cbfunc_motor[getPosition(RIGHT,20)]();
}

static void myCallbackRight21()
{
  cbfunc_motor[getPosition(RIGHT,21)]();
}

static void myCallbackRight22()
{
  cbfunc_motor[getPosition(RIGHT,22)]();
}

static void myCallbackRight23()
{
  cbfunc_motor[getPosition(RIGHT,23)]();
}

static void myCallbackRight24()
{
  cbfunc_motor[getPosition(RIGHT,24)]();
}

static void myCallbackRight25()
{
  cbfunc_motor[getPosition(RIGHT,25)]();
}

static void myCallbackRight26()
{
  cbfunc_motor[getPosition(RIGHT,26)]();
}

static void myCallbackRight27()
{
  cbfunc_motor[getPosition(RIGHT,27)]();
}

static void myCallbackRight28()
{
  cbfunc_motor[getPosition(RIGHT,28)]();
}

static void myCallbackRight29()
{
  cbfunc_motor[getPosition(RIGHT,29)]();
}

static void myCallbackRight30()
{
  cbfunc_motor[getPosition(RIGHT,30)]();
}

static void myCallbackRight31()
{
  cbfunc_motor[getPosition(RIGHT,31)]();
}

static void myCallbackRight32()
{
  cbfunc_motor[getPosition(RIGHT,32)]();
}

static void myCallbackRight33()
{
  cbfunc_motor[getPosition(RIGHT,33)]();
}

static void myCallbackRight34()
{
  cbfunc_motor[getPosition(RIGHT,34)]();
}

static void myCallbackRight35()
{
  cbfunc_motor[getPosition(RIGHT,35)]();
}

static void myCallbackRight36()
{
  cbfunc_motor[getPosition(RIGHT,36)]();
}

static void myCallbackRight37()
{
  cbfunc_motor[getPosition(RIGHT,37)]();
}

static void myCallbackRight38()
{
  cbfunc_motor[getPosition(RIGHT,38)]();
}

static void myCallbackRight39()
{
  cbfunc_motor[getPosition(RIGHT,39)]();
}

static void myCallbackRight40()
{
  cbfunc_motor[getPosition(RIGHT,40)]();
}

static fptr getCallBack(direction dir, int gpio)
{
	int position = getPosition(dir, gpio);

	switch (position)
	{
		case 0:	return &myCallbackLeft1;
		case 1:	return &myCallbackLeft2;
		case 2:	return &myCallbackLeft3;
		case 3:	return &myCallbackLeft4;
		case 4:	return &myCallbackLeft5;
		case 5:	return &myCallbackLeft6;
		case 6:	return &myCallbackLeft7;
		case 7:	return &myCallbackLeft8;
		case 8:	return &myCallbackLeft9;
		case 9:	return &myCallbackLeft10;
		case 10:return &myCallbackLeft11;
		case 11:return &myCallbackLeft12;
		case 12:return &myCallbackLeft13;
		case 13:return &myCallbackLeft14;
		case 14:return &myCallbackLeft15;
		case 15:return &myCallbackLeft16;
		case 16:return &myCallbackLeft17;
		case 17:return &myCallbackLeft18;
		case 18:return &myCallbackLeft19;
		case 19:return &myCallbackLeft20;
		case 20:return &myCallbackLeft21;
		case 21:return &myCallbackLeft22;
	    case 22:return &myCallbackLeft23;
		case 23:return &myCallbackLeft24;
		case 24:return &myCallbackLeft25;
		case 25:return &myCallbackLeft26;
		case 26:return &myCallbackLeft27;
		case 27:return &myCallbackLeft28;
		case 28:return &myCallbackLeft29;
		case 29:return &myCallbackLeft30;
		case 30:return &myCallbackLeft31;
		case 31:return &myCallbackLeft32;
		case 32:return &myCallbackLeft33;
		case 33:return &myCallbackLeft34;
		case 34:return &myCallbackLeft35;
		case 35:return &myCallbackLeft36;
		case 36:return &myCallbackLeft37;
		case 37:return &myCallbackLeft38;
		case 38:return &myCallbackLeft39;
		case 39:return &myCallbackLeft40;
		case 40:return &myCallbackRight1;
		case 41:return &myCallbackRight2;
		case 42:return &myCallbackRight3;
		case 43:return &myCallbackRight4;
		case 44:return &myCallbackRight5;
		case 45:return &myCallbackRight6;
		case 46:return &myCallbackRight7;
		case 47:return &myCallbackRight8;
		case 48:return &myCallbackRight9;
		case 49:return &myCallbackRight10;
		case 50:return &myCallbackRight11;
		case 51:return &myCallbackRight12;
		case 52:return &myCallbackRight13;
		case 53:return &myCallbackRight14;
		case 54:return &myCallbackRight15;
		case 55:return &myCallbackRight16;
		case 56:return &myCallbackRight17;
		case 57:return &myCallbackRight18;
		case 58:return &myCallbackRight19;
		case 59:return &myCallbackRight20;
		case 60:return &myCallbackRight21;
		case 61:return &myCallbackRight22;
	    case 62:return &myCallbackRight23;
		case 63:return &myCallbackRight24;
		case 64:return &myCallbackRight25;
		case 65:return &myCallbackRight26;
		case 66:return &myCallbackRight27;
		case 67:return &myCallbackRight28;
		case 68:return &myCallbackRight29;
		case 69:return &myCallbackRight30;
		case 70:return &myCallbackRight31;
		case 71:return &myCallbackRight32;
		case 72:return &myCallbackRight33;
		case 73:return &myCallbackRight34;
		case 74:return &myCallbackRight35;
		case 75:return &myCallbackRight36;
		case 76:return &myCallbackRight37;
		case 77:return &myCallbackRight38;
		case 78:return &myCallbackRight39;
		case 79:return &myCallbackRight40;
	}
}

/*
 * MotorFactory Constructor en Destructor
 */
MotorFactory::MotorFactory(){
    mfh = new MotorFactory::MotorFactoryHandler(*this);
	server->addHandler("/motorfactory", mfh);
	load();
}

MotorFactory::~MotorFactory(){
	delete mfh;
	std::map<std::string, MotorFactory::Motor*>::iterator it = motormap.begin();
	if (it != motormap.end())
	{
	    // found it - delete it
	    delete it->second;
	    motormap.erase(it);
	}
}

/*
 * MotorFactoryHandler Constructor en Destructor
 */
MotorFactory::MotorFactoryHandler::MotorFactoryHandler(MotorFactory& motorfactory):motorfactory(motorfactory){
}


MotorFactory::MotorFactoryHandler::~MotorFactoryHandler(){
}

/*
 * Motor Constructor en Destructor
 */
MotorFactory::Motor::Motor(std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay){
	mh = new MotorFactory::Motor::MotorHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;

	/*
	 * initialise gpio
	 */
	left_relay = GPIO_left_relay;
	left_sensor = GPIO_left_sensor;
	right_relay = GPIO_right_relay;
	right_sensor = GPIO_right_sensor;

	Initialize();

	std::stringstream ss;
	ss << "/motor-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

MotorFactory::Motor::Motor(std::string uuidstr, std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay){
	mh = new MotorFactory::Motor::MotorHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;

	/*
	 * initialise gpio
	 */
	left_relay = GPIO_left_relay;
	left_sensor = GPIO_left_sensor;
	right_relay = GPIO_right_relay;
	right_sensor = GPIO_right_sensor;

	Initialize();

	std::stringstream ss;
	ss << "/motor-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

MotorFactory::Motor::~Motor(){
	delete mh;
}

/*
 * Motor Handler Constructor en Destructor
 */
MotorFactory::Motor::MotorHandler::MotorHandler(MotorFactory::Motor& motor):motor(motor){
}

MotorFactory::Motor::MotorHandler::~MotorHandler(){
}


/* overige functies
 *
 */

void MotorFactory::load(){
	for (std::pair<std::string, MotorFactory::Motor*> element  : motormap)
	{
		deleteMotor(element.first);
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
		int left_relay = node[i]["left_relay"].as<int>();
		int right_relay = node[i]["right_relay"].as<int>();
		int left_sensor = node[i]["left_sensor"].as<int>();
		int right_sensor = node[i]["right_sensor"].as<int>();
		MotorFactory::Motor * motor = new MotorFactory::Motor(uuidstr, naam, omschrijving, left_sensor, right_sensor, left_relay, right_relay);
		std::string uuid_str = motor->getUuid();
		motormap.insert(std::make_pair(uuid_str,motor));
	}
}

void MotorFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, MotorFactory::Motor*>::iterator it = motormap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, MotorFactory::Motor*> element  : motormap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "left_relay";
		emitter << YAML::Value << element.second->left_relay;
		emitter << YAML::Key << "right_relay";
		emitter << YAML::Value << element.second->right_relay;
		emitter << YAML::Key << "left_sensor";
		emitter << YAML::Value << element.second->left_sensor;
		emitter << YAML::Key << "right_sensor";
		emitter << YAML::Value << element.second->right_sensor;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void MotorFactory::Motor::Initialize(){
	/*
	 * set relay to output and full stop
	 */
	pinMode(left_relay, OUTPUT);
	pinMode(right_relay, OUTPUT);
	digitalWrite(left_relay, HIGH);
	digitalWrite(right_relay, HIGH);
	pinMode(left_sensor, INPUT);
	pinMode(right_sensor, INPUT);
	pullUpDnControl(left_sensor, PUD_UP);
	pullUpDnControl(right_sensor, PUD_UP);

	/*
	 * Initialize callback functions to Dummy
	 */
	cbfunc_motor[getPosition(LEFT,this->left_sensor)] = std::bind(&Motor::Dummy,this);
	cbfunc_motor[getPosition(RIGHT,this->right_sensor)] = std::bind(&Motor::Dummy,this);
	if ( myWiringPiISR (left_sensor, INT_EDGE_BOTH, LEFT) < 0 ) {
		 std::cerr << "Error setting interrupt for left GPIO sensor " << std::endl;
	 }
	if ( myWiringPiISR (right_sensor, INT_EDGE_BOTH, RIGHT) < 0 ) {
	     std::cerr << "Error setting interrupt for right GPIO sensor " << std::endl;
	}
}

std::string MotorFactory::Motor::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string MotorFactory::Motor::getUrl(){
	return url;
}

MotorFactory::Motor* MotorFactory::addMotor(std::string naam, std::string omschrijving, int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay){
	MotorFactory::Motor * motor = new MotorFactory::Motor(naam, omschrijving, GPIO_left_sensor, GPIO_right_sensor, GPIO_left_relay, GPIO_right_relay);
	std::string uuid_str = motor->getUuid();
	motormap.insert(std::make_pair(uuid_str,motor));
	return motor;
}

void MotorFactory::deleteMotor(std::string uuid){
	std::map<std::string, MotorFactory::Motor*>::iterator it = motormap.begin();
    it = motormap.find(uuid);
	if (it != motormap.end())
	{
	    // found it - delete it
	    delete it->second;
	    motormap.erase(it);
	}
}

int MotorFactory::Motor::myWiringPiISR(int val, int mask, direction dir)
{
  switch (dir)
  {
  	  case LEFT:
  		  return wiringPiISR(val, mask, getCallBack(LEFT, this->left_sensor));
  	  case RIGHT:
  		  return wiringPiISR(val, mask, getCallBack(RIGHT, this->right_sensor));
  	  default:
  		  std::cerr << "Invalid direction specified on myWiringPiISR method call!!" << std::endl;
  		  return -1;
  }
}

void MotorFactory::Motor::Wait(){
	while (!(full_left||full_right))
	{
		delay(200);
	}
}

bool MotorFactory::Motor::Full(direction dir){
	switch (dir){
		case LEFT:
			return(full_left);
		case RIGHT:
			return(full_right);
		default:
			std::cerr << "Invalid direction specified on Full method call!!" << std::endl;
			return false;
	}
}

void MotorFactory::Motor::Stop(){
	digitalWrite(left_relay, HIGH);
	digitalWrite(right_relay, HIGH);
	delay(500);
}

void MotorFactory::Motor::Stop(direction dir){
		switch (dir){
		case LEFT:
			pinMode(left_sensor, INPUT);
			if (!(digitalRead(left_sensor))){
			   /*
			    * motor is NOT at the left sensor! This is unexpected! Full stop and error message!
			    */
     			Stop();
				std::cerr << "Motor should be at the left, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(left_relay, HIGH);
				full_left=true;
				cbfunc_motor[getPosition(LEFT,this->left_sensor)] = std::bind(&Motor::Dummy,this);
			}
		break;
		case RIGHT:
			pinMode(right_sensor, INPUT);
		    if (!(digitalRead(right_sensor))){
			   /*
			    * motor is NOT at the right sensor! This is unexpected! Full stop and error message!
			    */
			    Stop();
				std::cerr << "Motor should be at the right, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(right_relay, HIGH);
			    full_right=true;
			    cbfunc_motor[getPosition(RIGHT,this->right_sensor)] = std::bind(&Motor::Dummy,this);
			}
		break;
		default:
			std::cerr << "Unknown state received in Stop function!" << std::endl;
	}
}

void MotorFactory::Motor::Start(direction dir){
	switch (dir){
			case LEFT:
				pinMode(left_sensor, INPUT);
				if (!(digitalRead(left_sensor))){
						/*
						 * motor is not already in left position, so we should start moving left
						 * after we set the interrupt handler for the sensor.
						 */
						Stop();
						full_right=false;
						full_left=false;
						cbfunc_motor[getPosition(LEFT,this->left_sensor)] = std::bind(static_cast<void (Motor::*)(direction)>(&Motor::Stop),this,LEFT);
						digitalWrite(left_relay, LOW);
				}
				else full_left=true;
			break;
			case RIGHT:
				pinMode(right_sensor, INPUT);
				if (!(digitalRead(right_sensor))){
						/*
						 * motor is not already in right position, so we should start moving right
						 * after we set the interrupt handler for the sensor.
						 */
						Stop();
						full_left=false;
						full_right=false;
						cbfunc_motor[getPosition(RIGHT,this->right_sensor)] = std::bind(static_cast<void (Motor::*)(direction)>(&Motor::Stop),this,RIGHT);
						digitalWrite(right_relay, LOW);
				}
				else full_right=true;
			break;
			default:
				std::cerr << "Unknown state received in Start function!" << std::endl;
		}
}

void MotorFactory::Motor::Dummy(){
}

std::string MotorFactory::Motor::getNaam(){
	return naam;
}

std::string MotorFactory::Motor::getOmschrijving(){
	return omschrijving;
}

bool MotorFactory::MotorFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return MotorFactory::MotorFactoryHandler::handleAll("GET", server, conn);
	}

bool MotorFactory::MotorFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return MotorFactory::MotorFactoryHandler::handleAll("POST", server, conn);
	}

bool MotorFactory::Motor::MotorHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return MotorFactory::Motor::MotorHandler::handleAll("GET", server, conn);
	}

bool MotorFactory::Motor::MotorHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return MotorFactory::Motor::MotorHandler::handleAll("POST", server, conn);
	}

bool MotorFactory::MotorFactoryHandler::handleAll(const char *method,
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
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/motorfactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->motorfactory.deleteMotor(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/motorfactory\" /></head><body>");
		mg_printf(conn, "<h2>Motor opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->motorfactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/motorfactory\" /></head><body>");
		mg_printf(conn, "<h2>Motor ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->motorfactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;
		CivetServer::getParam(conn, "left_relay", value);
		int left_relay = atoi(value.c_str());
		CivetServer::getParam(conn, "right_relay", value);
		int right_relay = atoi(value.c_str());
		CivetServer::getParam(conn, "left_sensor", value);
		int left_sensor = atoi(value.c_str());
		CivetServer::getParam(conn, "right_sensor", value);
		int right_sensor = atoi(value.c_str());

		MotorFactory::Motor* motor = motorfactory.addMotor(naam, omschrijving, left_sensor, right_sensor, left_relay, right_relay);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << motor->getUrl() << "\"/></head><body>";
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
	   ss << "<form action=\"/motorfactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<label for=\"left_sensor\">Linker sensor GPIO:</label>"
	   	     "<input id=\"left_sensor\" type=\"text\" size=\"3\" name=\"left_sensor\"/>" << "</br>";
	   ss << "<label for=\"right_sensor\">Rechter sensor GPIO:</label>"
	   	     "<input id=\"right_sensor\" type=\"text\" size=\"3\" name=\"right_sensor\"/>" << "</br>";
	   ss << "<label for=\"left_relay\">Linker relais GPIO:</label>"
	   	     "<input id=\"left_relay\" type=\"text\" size=\"3\" name=\"left_relay\"/>" << "</br>";
	   ss << "<label for=\"right_relay\">Rechter relais GPIO:</label>"
	   	     "<input id=\"right_relay\" type=\"text\" size=\"3\" name=\"right_relay\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
   	   ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
       ss <<  "</br>";
       ss << "<a href=\"/motorfactory\">Motoren</a>";
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
		std::map<std::string, MotorFactory::Motor*>::iterator it = motorfactory.motormap.begin();
		ss << "<h2>Beschikbare motoren:</h2>";
	    for (std::pair<std::string, MotorFactory::Motor*> element : motorfactory.motormap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/motorfactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/motorfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/motorfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/motorfactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool MotorFactory::Motor::MotorHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string param = "chan";
	mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n");

	/* if parameter submit is present the submit button was pushed */
	if(CivetServer::getParam(conn, "submit", dummy))
	{
	   CivetServer::getParam(conn,"left-sensor", s[0]);
	   motor.left_sensor = atoi(s[0].c_str());
	   CivetServer::getParam(conn,"right-sensor", s[1]);
	   motor.right_sensor = atoi(s[1].c_str());
	   CivetServer::getParam(conn,"left-relay", s[2]);
	   motor.left_relay = atoi(s[2].c_str());
	   CivetServer::getParam(conn,"right-relay", s[3]);
	   motor.right_relay = atoi(s[3].c_str());
	   CivetServer::getParam(conn,"naam", s[4]);
	   motor.naam = s[4].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[5]);
	   motor.omschrijving = s[5].c_str();

	   motor.Initialize();

	   std::stringstream ss;
	   ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << motor.getUrl() << "\"/></head><body>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	/* if parameter left is present left button was pushed */
	else if(CivetServer::getParam(conn, "left", dummy))
	{
		motor.Start(LEFT);
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << motor.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Naar links...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		motor.Stop();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << motor.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stoppen...!</h2>");
	}
	/* if parameter right is present right button was pushed */
	else if(CivetServer::getParam(conn, "right", dummy))
	{
		motor.Start(RIGHT);
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << motor.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Naar rechts...!</h2>");
	}
	else
	{
		mg_printf(conn, "<h2>&nbsp;</h2>");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	}
	/* initial page display */
	{
		std::stringstream ss;
		ss << "<h2>Motor:</h2>";
		ss << "<form action=\"" << motor.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  motor.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  motor.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
	    ss << "Huidige status sensor links:&nbsp;" << digitalRead(motor.left_sensor) << "<br>";
	    ss << "Huidige status sensor rechts:&nbsp;" << digitalRead(motor.right_sensor) << "<br>";
	    ss << "Huidige status relais links:&nbsp;" << digitalRead(motor.left_relay) << "<br>";
	    ss << "Huidige status relais rechts:&nbsp;" << digitalRead(motor.right_relay) << "<br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"left\" value=\"left\" id=\"left\">LINKS</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
	    ss << "<button type=\"submit\" name=\"right\" value=\"right\" id=\"right\">RECHTS</button></br>";
		ss << "<h2>GPIO pins:</h2>";
		ss << "<label for=\"left-sensor\">Left Sensor GPIO pin:</label>"
			  "<input id=\"left-sensor\" type=\"text\" size=\"4\" value=\"" <<
			  motor.left_sensor << "\" name=\"left-sensor\"/>" << "</br>";
	    ss << "<label for=\"right-sensor\">Right Sensor GPIO pin:</label>"
	          "<input id=\"right-sensor\" type=\"text\" size=\"4\" value=\"" <<
	    	  motor.right_sensor << "\" name=\"right-sensor\"/>" << "</br>";
	    ss << "<label for=\"left-relay\">Left Relay GPIO pin:</label>"
	    	  "<input id=\"left-relay type=\"text\" size=\"4\" value=\"" <<
	    	  motor.left_relay << "\" name=\"left-relay\"/>" << "</br>";
	    ss << "<label for=\"right-relay\">Right Relay GPIO pin:</label>"
	    	  "<input id=\"right-relay\" type=\"text\" size=\"4\" value=\"" <<
	    	  motor.right_relay << "\" name=\"right-relay\"/>" << "</br>";
	    ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<img src=\"images/RP2_Pinout.png\" alt=\"Pin Layout\" style=\"width:400px;height:300px;\"><br>";
	    ss << "<a href=\"/motorfactory\">Motoren</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

