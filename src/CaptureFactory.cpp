/*
 * CaptureFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "CaptureFactory.hpp"
#include <b64/encode.h>
using namespace std;
using namespace cv;

/*
 * CaptureFactory Constructor en Destructor
 */
CaptureFactory::CaptureFactory(){
    mfh = new CaptureFactory::CaptureFactoryHandler(*this);
	server->addHandler("/capturefactory", mfh);
	load();
}

CaptureFactory::~CaptureFactory(){
	delete mfh;
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();
	if (it != capturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    capturemap.erase(it);
	}
}

/*
 * CaptureFactoryHandler Constructor en Destructor
 */
CaptureFactory::CaptureFactoryHandler::CaptureFactoryHandler(CaptureFactory& capturefactory):capturefactory(capturefactory){
}


CaptureFactory::CaptureFactoryHandler::~CaptureFactoryHandler(){
}

/*
 * Capture Constructor en Destructor
 */
CaptureFactory::Capture::Capture(std::string naam, std::string omschrijving){
	mh = new CaptureFactory::Capture::CaptureHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;

	Initialize();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::Capture(std::string uuidstr, std::string naam, std::string omschrijving){
	mh = new CaptureFactory::Capture::CaptureHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;

	Initialize();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::~Capture(){
	delete mh;
	delete cap;
}

/*
 * Capture Handler Constructor en Destructor
 */
CaptureFactory::Capture::CaptureHandler::CaptureHandler(CaptureFactory::Capture& capture):capture(capture){
}

CaptureFactory::Capture::CaptureHandler::~CaptureHandler(){
}


/* overige functies
 *
 */

void CaptureFactory::load(){
	for (std::pair<std::string, CaptureFactory::Capture*> element  : capturemap)
	{
		deleteCapture(element.first);
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
		CaptureFactory::Capture * capture = new CaptureFactory::Capture(uuidstr, naam, omschrijving);
		std::string uuid_str = capture->getUuid();
		capturemap.insert(std::make_pair(uuid_str,capture));
	}
}

void CaptureFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, CaptureFactory::Capture*> element  : capturemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void CaptureFactory::Capture::Initialize(){
	/*
	 * set relay to output and full stop
	 */
	cv::VideoCapture* cap = new cv::VideoCapture();
	cv::Mat input;
    // Serialize the input image to a stringstream
	cout << "Grabbing a frame..." << endl;
	cap->set(CV_CAP_PROP_FRAME_WIDTH,1920);   // width pixels
	cap->set(CV_CAP_PROP_FRAME_HEIGHT,1080);   // height pixels
	cap->open(0);
	if(!cap->isOpened()){   // connect to the camera
	         cout << "Failed to connect to the camera." << endl;
	         return;
    }
    // Grab a frame
    *cap >> input;
    cap->release();

    std::vector<uchar> buf;
    cv::imencode(".jpg", input, buf, std::vector<int>() );

    // Base64 encode the stringstream
    base64::encoder E;
    stringstream encoded;
    stringstream incoming;
    std::ofstream FILE("test.jpg", std::ios::out | std::ofstream::binary);
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    E.encode(incoming, encoded);

    serializedStream << encoded.rdbuf();
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */

}

std::string CaptureFactory::Capture::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string CaptureFactory::Capture::getUrl(){
	return url;
}

CaptureFactory::Capture* CaptureFactory::addCapture(std::string naam, std::string omschrijving){
	CaptureFactory::Capture * capture = new CaptureFactory::Capture(naam, omschrijving);
	std::string uuid_str = capture->getUuid();
	capturemap.insert(std::make_pair(uuid_str,capture));
	return capture;
}

void CaptureFactory::deleteCapture(std::string uuid){
	std::map<std::string, CaptureFactory::Capture*>::iterator it = capturemap.begin();
    it = capturemap.find(uuid);
	if (it != capturemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    capturemap.erase(it);
	}
}

void CaptureFactory::Capture::Stop(){
	delay(500);
}

void CaptureFactory::Capture::Start(){
	delay(500);
}

std::string CaptureFactory::Capture::getNaam(){
	return naam;
}

std::string CaptureFactory::Capture::getOmschrijving(){
	return omschrijving;
}

bool CaptureFactory::CaptureFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::CaptureFactoryHandler::handleAll("GET", server, conn);
	}

bool CaptureFactory::CaptureFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::CaptureFactoryHandler::handleAll("POST", server, conn);
	}

bool CaptureFactory::Capture::CaptureHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::Capture::CaptureHandler::handleAll("GET", server, conn);
	}

bool CaptureFactory::Capture::CaptureHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CaptureFactory::Capture::CaptureHandler::handleAll("POST", server, conn);
	}

bool CaptureFactory::CaptureFactoryHandler::handleAll(const char *method,
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
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/capturefactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->capturefactory.deleteCapture(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" /></head><body>");
		mg_printf(conn, "<h2>Capture opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->capturefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/capturefactory\" /></head><body>");
		mg_printf(conn, "<h2>Capture ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->capturefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;


		CaptureFactory::Capture* capture = capturefactory.addCapture(naam, omschrijving);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << capture->getUrl() << "\"/></head><body>";
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
	   ss << "<form action=\"/capturefactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
       ss << "<a href=\"/capturefactory\">Aan/Uit</a>";
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
		std::map<std::string, CaptureFactory::Capture*>::iterator it = capturefactory.capturemap.begin();
		ss << "<h2>Beschikbare Aan/Uit:</h2>";
	    for (std::pair<std::string, CaptureFactory::Capture*> element : capturefactory.capturemap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/capturefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool CaptureFactory::Capture::CaptureHandler::handleAll(const char *method,
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
	   CivetServer::getParam(conn,"naam", s[0]);
	   capture.naam = s[0].c_str();
	   CivetServer::getParam(conn,"omschrijving", s[1]);
	   capture.omschrijving = s[1].c_str();

	   capture.Initialize();

	   std::stringstream ss;
	   ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "start", dummy))
	{
		capture.Start();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Starten...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "stop", dummy))
	{
		capture.Stop();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stoppen...!</h2>");
	}
	else
	{
		mg_printf(conn, "<h2>&nbsp;</h2>");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	}
	/* initial page display */
	{
		std::stringstream ss;
		ss << "<h2>Capture:</h2>";
		ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" size=\"10\" value=\"" <<
					  capture.naam << "\" name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"omschrijving\" type=\"text\" size=\"20\" value=\"" <<
					  capture.omschrijving << "\" name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">Refresh</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"start\" value=\"start\" id=\"start\">START</button>";
	    ss << "<button type=\"submit\" name=\"stop\" value=\"stop\" id=\"stop\">STOP</button>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<img alt=\"Embedded Image\" src=\"data:image/jpeg;base64," << capture.serializedStream.rdbuf() << "\"/><br>";
	    ss << "<a href=\"/capturefactory\">Capture</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

