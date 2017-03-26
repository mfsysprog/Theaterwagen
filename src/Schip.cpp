/*
 * Schip.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "Schip.h"

/*
 * The wiringPiISR function expects a c function for the callback. Since we cannot directly call a member function
 * (c is not aware of 'non-static' methods) we need a little trick.
 * The myWiringPiISR sets the callback to myCallbackLeft and myCallbackRight.
 * These functions use cbfunc_schip to point to a std::function object that 'wraps' the actual
 * member function and its argument we want to use for the callback.
 * The object is created by a std::bind, that adds the 'this' pointer as the
 * first (hidden) argument on the function call to the actual callback method and then adds the enum direction
 * as the first true argument to the call.
 */

static std::function<void()> cbfunc_schip[2];

static void myCallbackLeft()
{
  cbfunc_schip[LEFT]();
}

static void myCallbackRight()
{
  cbfunc_schip[RIGHT]();
}

void Schip::Dummy(){
	std::cout << "+";
}

Schip::SchipHandler::SchipHandler(Schip& schip):schip(schip){
}

CivetHandler& Schip::getHandler(){
	return sh;
}

bool Schip::SchipHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}

bool Schip::SchipHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}

bool Schip::SchipHandler::handleAll(const char *method,
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
	   schip.left_sensor = atoi(s[0].c_str());
	   CivetServer::getParam(conn,"right-sensor", s[1]);
	   schip.right_sensor = atoi(s[1].c_str());
	   CivetServer::getParam(conn,"left-relay", s[2]);
	   schip.left_relay = atoi(s[2].c_str());
	   CivetServer::getParam(conn,"right-relay", s[3]);
	   schip.right_relay = atoi(s[3].c_str());


	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/schip\" /></head><body>");
	   mg_printf(conn, "<h2>Changes committed...!</h2>");
	}
	/* if parameter left is present left button was pushed */
	else if(CivetServer::getParam(conn, "left", dummy))
	{
		schip.Start(LEFT);
     	mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/schip\" /></head><body>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		schip.Stop();
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/schip\" /></head><body>");
	}
	/* if parameter right is present right button was pushed */
	else if(CivetServer::getParam(conn, "right", dummy))
	{
		schip.Start(RIGHT);
     	mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/schip\" /></head><body>");
	}
	/* initial page display */
	else
	{
		mg_printf(conn, "<html><body>");
		if(schip.full_left)
			mg_printf(conn, "Schip is volledig links.</br>");
		else if (schip.full_right)
			mg_printf(conn, "Schip is volledig rechts.</br>");
		mg_printf(conn, "<form action=\"/schip\" method=\"post\">");
		std::stringstream ss;
	    ss << "<h2>Status:</h2>";
	    ss << "Huidige status sensor links:&nbsp;" << digitalRead(schip.left_sensor) << "</br>";
	    ss << "Huidige status sensor rechts:&nbsp;" << digitalRead(schip.right_sensor) << "</br>";
	    ss << "Huidige status relais links:&nbsp;" << digitalRead(schip.left_relay) << "</br>";
	    ss << "Huidige status relais rechts:&nbsp;" << digitalRead(schip.right_relay) << "</br>";
	    ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"left\" value=\"left\" id=\"left\">LINKS</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
	    ss << "<button type=\"submit\" name=\"right\" value=\"right\" id=\"right\">RECHTS</button></br>";
		ss << "<h2>GPIO pins:</h2>";
		ss << "<label for=\"left-sensor\">Left Sensor GPIO pin:</label>"
			  "<input id=\"left-sensor type=\"text\" size=\"4\" value=\"" <<
			  schip.left_sensor << "\" name=\"left-sensor\"/>" << "</br>";
	    ss << "<label for=\"right-sensor\">Right Sensor GPIO pin:</label>"
	          "<input id=\"right-sensor type=\"text\" size=\"4\" value=\"" <<
	    	  schip.right_sensor << "\" name=\"right-sensor\"/>" << "</br>";
	    ss << "<label for=\"left-relay\">Left Relay GPIO pin:</label>"
	    	  "<input id=\"left-relay type=\"text\" size=\"4\" value=\"" <<
	    	  schip.left_relay << "\" name=\"left-relay\"/>" << "</br>";
	    ss << "<label for=\"right-relay\">Right Relay GPIO pin:</label>"
	    	  "<input id=\"right-relay type=\"text\" size=\"4\" value=\"" <<
	    	  schip.right_relay << "\" name=\"right-relay\"/>" << "</br>";
	    ss <<  "</br>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

int Schip::myWiringPiISR(int val, int mask, direction dir)
{
  switch (dir)
  {
  	  case LEFT:
  		  return wiringPiISR(val, mask, &myCallbackLeft);
  	  case RIGHT:
  		  return wiringPiISR(val, mask, &myCallbackRight);
  	  default:
  		  std::cerr << "Invalid direction specified on myWiringPiISR method call!!" << std::endl;
  		  return -1;
  }
}

bool Schip::Full(direction dir){
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

void Schip::Stop(){
	digitalWrite(left_relay, HIGH);
	digitalWrite(right_relay, HIGH);
	delay(1000);
}

void Schip::Stop(direction dir){
		switch (dir){
		case LEFT:
			pinMode(left_sensor, INPUT);
			if (!(digitalRead(left_sensor))){
			   /*
			    * motor is NOT at the left sensor! This is unexpected! Full stop and error message!
			    */
     			Stop();
				std::cerr << "Schip should be at the left, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(left_relay, HIGH);
				full_left=true;
				cbfunc_schip[LEFT] = std::bind(&Schip::Dummy,this);
			}
		break;
		case RIGHT:
			pinMode(right_sensor, INPUT);
		    if (!(digitalRead(right_sensor))){
			   /*
			    * motor is NOT at the right sensor! This is unexpected! Full stop and error message!
			    */
			    Stop();
				std::cerr << "Schip should be at the right, but it is not!!!" << std::endl;
			}
			else {
				digitalWrite(right_relay, HIGH);
			    full_right=true;
			    cbfunc_schip[RIGHT] = std::bind(&Schip::Dummy,this);
			}
		break;
		default:
			std::cerr << "Unknown state received in Stop function!" << std::endl;
	}
}

void Schip::Start(direction dir){
	switch (dir){
			case LEFT:
				std::cout << "Start left called!" << std::endl;
				pinMode(left_sensor, INPUT);
				if (!(digitalRead(left_sensor))){
						/*
						 * motor is not already in left position, so we should start moving left
						 * after we set the interrupt handler for the sensor.
						 */
						Stop();
						full_left=false;
						cbfunc_schip[LEFT] = std::bind(static_cast<void (Schip::*)(direction)>(&Schip::Stop),this,LEFT);
						digitalWrite(left_relay, LOW);
				}
				else full_left=true;
			break;
			case RIGHT:
				std::cout << "Start right called!" << std::endl;
				pinMode(right_sensor, INPUT);
				if (!(digitalRead(right_sensor))){
						/*
						 * motor is not already in right position, so we should start moving right
						 * after we set the interrupt handler for the sensor.
						 */
						Stop();
						full_right=false;
						cbfunc_schip[RIGHT] = std::bind(static_cast<void (Schip::*)(direction)>(&Schip::Stop),this,RIGHT);
						digitalWrite(right_relay, LOW);
				}
				else full_right=true;
			break;
			default:
				std::cerr << "Unknown state received in Start function!" << std::endl;
		}
}

Schip::Schip(int GPIO_left_sensor, int GPIO_right_sensor, int GPIO_left_relay, int GPIO_right_relay):sh(*this) {
	/*
	 * initialise gpio
	 */
	left_relay = GPIO_left_relay;
	left_sensor = GPIO_left_sensor;
	right_relay = GPIO_right_relay;
	right_sensor = GPIO_right_sensor;
	/*
	 * set relay to output and full stop
	 */
	pinMode(left_relay, OUTPUT);
	pinMode(right_relay, OUTPUT);
	digitalWrite(left_relay, HIGH);
	digitalWrite(right_relay, HIGH);
	/*
	 * Initialize callback functions to Dummy
	 */
	cbfunc_schip[LEFT] = std::bind(&Schip::Dummy,this);
	cbfunc_schip[RIGHT] = std::bind(&Schip::Dummy,this);
	if ( myWiringPiISR (left_sensor, INT_EDGE_FALLING, LEFT) < 0 ) {
		 std::cerr << "Error setting interrupt for left GPIO sensor " << std::endl;
	 }
	if ( myWiringPiISR (right_sensor, INT_EDGE_FALLING, RIGHT) < 0 ) {
	     std::cerr << "Error setting interrupt for right GPIO sensor " << std::endl;
	}
}

Schip::~Schip() {
	// TODO Auto-generated destructor stub
}

