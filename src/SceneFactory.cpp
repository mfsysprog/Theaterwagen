/*
 * SceneFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "SceneFactory.hpp"
#include "FixtureFactory.hpp"
#include "usb.h"
#include "uDMX_cmds.h"

#define USBDEV_SHARED_VENDOR 0x16C0  /* Obdev's free shared VID */
#define USBDEV_SHARED_PRODUCT 0x05DC /* Obdev's free shared PID */
/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */

int debug = 0;
int verbose = 0;
usb_dev_handle *handle = NULL;

static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid,
                             char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
                                (USB_DT_STRING << 8) + index, langid, buffer,
                                sizeof(buffer), 1000)) < 0)
        return rval;
    if (buffer[1] != USB_DT_STRING)
        return 0;
    if ((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for (i = 1; i < rval; i++) {
        if (i > buflen) /* destination buffer overflow */
            break;
        buf[i - 1] = buffer[2 * i];
        if (buffer[2 * i + 1] != 0) /* outside of ISO Latin1 range */
            buf[i - 1] = '?';
    }
    buf[i - 1] = 0;
    return i - 1;
}

/*
 * uDMX uses the free shared default VID/PID.
 * To avoid talking to some other device we check the vendor and
 * device strings returned.
 */
static usb_dev_handle *findDevice(void) {
    struct usb_bus *bus;
    struct usb_device *dev;
    char string[256];
    int len;
    usb_dev_handle *handle = 0;

    usb_find_busses();
    usb_find_devices();
    for (bus = usb_busses; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == USBDEV_SHARED_VENDOR &&
                dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT) {
                if (debug) { printf("Found device with %x:%x\n",
                               USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT); }

                /* open the device to query strings */
                handle = usb_open(dev);
                if (!handle) {
                    fprintf(stderr, "Warning: cannot open USB device: %s\n",
                            usb_strerror());
                    continue;
                }

                /* now find out whether the device actually is a uDMX */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer,
                                        0x0409, string, sizeof(string));
                if (len < 0) {
                    fprintf(stderr, "warning: cannot query manufacturer for device: %s\n",
                            usb_strerror());
                    goto skipDevice;
                }
                if (debug) { printf("Device vendor is %s\n",string); }
                if (strcmp(string, "www.anyma.ch") != 0)
                    goto skipDevice;

                len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
                if (len < 0) {
                    fprintf(stderr, "warning: cannot query product for device: %s\n", usb_strerror());
                    goto skipDevice;
                }
                if (debug) { printf("Device product is %s\n",string); }
                if (strcmp(string, "uDMX") == 0)
                    break;

            skipDevice:
                usb_close(handle);
                handle = NULL;
            }
        }
        if (handle)
            break;
    }
    return handle;
}


/*
 * SceneFactory Constructor en Destructor
 */
SceneFactory::SceneFactory(FixtureFactory* ff){
    mfh = new SceneFactory::SceneFactoryHandler(*this);
    this->ff = ff;
	server->addHandler("/scenefactory", mfh);
    usb_set_debug(0);

    usb_init();
    handle = findDevice();

    if (handle == NULL) {
        fprintf(stderr,
                "Could not find USB device www.anyma.ch/uDMX (vid=0x%x pid=0x%x)\n",
                USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        //exit(1);
    }
}

SceneFactory::~SceneFactory(){
	delete mfh;
	std::map<std::string, SceneFactory::Scene*>::iterator it = scenemap.begin();
	if (it != scenemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    scenemap.erase(it);
	}
    usb_close(handle);
}

/*
 * SceneFactoryHandler Constructor en Destructor
 */
SceneFactory::SceneFactoryHandler::SceneFactoryHandler(SceneFactory& scenefactory):scenefactory(scenefactory){
}


SceneFactory::SceneFactoryHandler::~SceneFactoryHandler(){
}

/*
 * Scene Constructor en Destructor
 */
SceneFactory::Scene::Scene(FixtureFactory* ff, std::string naam, std::string omschrijving){
	mh = new SceneFactory::Scene::SceneHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->channels = new unsigned char[512]{0};
	this->ff = ff;
	this->naam = naam;
	this->omschrijving = omschrijving;
	std::stringstream ss;
	ss << "/scene-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

SceneFactory::Scene::Scene(FixtureFactory* ff, std::string uuidstr, std::string naam, std::string omschrijving, unsigned char* channels){
	mh = new SceneFactory::Scene::SceneHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->channels = channels;
	this->ff = ff;
	this->naam = naam;
	this->omschrijving = omschrijving;
	std::stringstream ss;
	ss << "/scene-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

SceneFactory::Scene::~Scene(){
	delete mh;
	delete channels;
}

/*
 * Scene Handler Constructor en Destructor
 */
SceneFactory::Scene::SceneHandler::SceneHandler(SceneFactory::Scene& scene):scene(scene){
}

SceneFactory::Scene::SceneHandler::~SceneHandler(){
}


/* overige functies
 *
 */

void SceneFactory::load(){
	for (std::pair<std::string, SceneFactory::Scene*> element  : scenemap)
	{
		deleteScene(element.first);
	}
	YAML::Node node = YAML::LoadFile(CONFIG_FILE);
	assert(node.IsSequence());
	for (std::size_t i=0;i<node.size();i++) {
		assert(node[i].IsMap());
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		unsigned char* channels = new unsigned char[512];
		for (std::size_t k=0;k < 512;k++)
		{
			//fprintf(stderr,"Value for channel %i:  %u\n",k,atoi(node[i]["channels"][k].as<std::string>().c_str()));
			channels[k] = atoi(node[i]["channels"][k].as<std::string>().c_str());
		}
		SceneFactory::Scene * scene = new SceneFactory::Scene(ff, uuidstr, naam, omschrijving, channels);
		std::string uuid_str = scene->getUuid();
		scenemap.insert(std::make_pair(uuid_str,scene));
	}
}

void SceneFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE);
	std::map<std::string, SceneFactory::Scene*>::iterator it = scenemap.begin();

	emitter << YAML::BeginSeq;
	for (std::pair<std::string, SceneFactory::Scene*> element  : scenemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "channels";
		emitter << YAML::BeginSeq;
		for (unsigned int i = 0; i < 512 ; i++)
		{
			emitter << YAML::Value << std::to_string(element.second->channels[i]);
		}
		emitter << YAML::EndSeq;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}


std::string SceneFactory::Scene::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string SceneFactory::Scene::getUrl(){
	return url;
}

void SceneFactory::Scene::Play(){
	int nBytes;
	nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
		                                    cmd_SetChannelRange, 512, 0, (char *)this->channels, 512, 1000);
	if (nBytes < 0)
	   fprintf(stderr, "USB error: %s\n", usb_strerror());
}

SceneFactory::Scene* SceneFactory::addScene(std::string naam, std::string omschrijving){
	SceneFactory::Scene * scene = new SceneFactory::Scene(ff, naam, omschrijving);
	std::string uuid_str = scene->getUuid();
	scenemap.insert(std::make_pair(uuid_str,scene));
	return scene;
}

void SceneFactory::deleteScene(std::string uuid){
	std::map<std::string, SceneFactory::Scene*>::iterator it = scenemap.begin();
    it = scenemap.find(uuid);
	if (it != scenemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    scenemap.erase(it);
	}
}

std::string SceneFactory::Scene::getNaam(){
	return naam;
}

std::string SceneFactory::Scene::getOmschrijving(){
	return omschrijving;
}


bool SceneFactory::SceneFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return SceneFactory::SceneFactoryHandler::handleAll("GET", server, conn);
	}

bool SceneFactory::SceneFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return SceneFactory::SceneFactoryHandler::handleAll("POST", server, conn);
	}

bool SceneFactory::Scene::SceneHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return SceneFactory::Scene::SceneHandler::handleAll("GET", server, conn);
	}

bool SceneFactory::Scene::SceneHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return SceneFactory::Scene::SceneHandler::handleAll("POST", server, conn);
	}

bool SceneFactory::SceneFactoryHandler::handleAll(const char *method,
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
	   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/scenefactory\" /></head><body>");
	   mg_printf(conn, "</body></html>");
	   this->scenefactory.deleteScene(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/scenefactory\" /></head><body>");
		mg_printf(conn, "<h2>Scene opgeslagen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->scenefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/scenefactory\" /></head><body>");
		mg_printf(conn, "<h2>Scene ingeladen...!</h2>");
		mg_printf(conn, "</body></html>");
		this->scenefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
		CivetServer::getParam(conn, "naam", value);
		std::string naam = value;
		CivetServer::getParam(conn, "omschrijving", value);
		std::string omschrijving = value;

		SceneFactory::Scene* scene = scenefactory.addScene(naam, omschrijving);

		mg_printf(conn,
				          "HTTP/1.1 200 OK\r\nContent-Type: "
				          "text/html\r\nConnection: close\r\n\r\n");
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << scene->getUrl() << "\"/></head><body>";
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
	   ss << "<form action=\"/scenefactory\" method=\"POST\">";
	   ss << "<label for=\"naam\">Naam:</label>"
  			 "<input id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">Omschrijving:</label>"
	         "<input id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">Toevoegen</button>&nbsp;";
   	   ss << "</form>";
       ss <<  "</br>";
       ss << "<a href=\"/scenefactory\">Scenes</a>";
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
		std::map<std::string, SceneFactory::Scene*>::iterator it = scenefactory.scenemap.begin();
		ss << "<h2>Beschikbare scenes:</h2>";
	    for (std::pair<std::string, SceneFactory::Scene*> element : scenefactory.scenemap) {
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">Selecteren</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->getUuid() << "\" id=\"delete\">Verwijderen</button>&nbsp;";
			ss << "Naam:&nbsp;" << element.second->getNaam() << "&nbsp;";
			ss << "Omschrijving:&nbsp;" << element.second->getOmschrijving();
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Nieuw</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">Opslaan</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">Laden</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

bool SceneFactory::Scene::SceneHandler::handleAll(const char *method,
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
		if(CivetServer::getParam(conn,"naam", value))
		  		scene.naam = value;
		if(CivetServer::getParam(conn,"omschrijving", value))
		  		scene.omschrijving = value;
		std::stringstream ss;
		for (std::pair<int, FixtureFactory::Fixture*> element : scene.ff->fixturemap) {
			for (int i = element.second->base_channel; i < element.second->base_channel + element.second->number_channels; i++)
			{
				std::stringstream channel;
				channel << "chan" << i;
				CivetServer::getParam(conn,channel.str().c_str(), value);
				scene.channels[i-1] = atoi(value.c_str());
		  	}
		}
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << scene.getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	} else
	if(CivetServer::getParam(conn, "play", dummy))
	{
		scene.Play();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << scene.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Playing...!</h2>");
	} else
	{
		mg_printf(conn, "<h2>&nbsp;</h2>");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	}
	{
		std::stringstream ss;
		ss << "<h2>Scenes:</h2>";
		ss << "<form action=\"" << scene.getUrl() << "\" method=\"POST\">";
		ss << "<label for=\"naam\">Naam:</label>"
					  "<input id=\"naam\" type=\"text\" value=\"" <<
					  scene.getNaam() << "\"" << " name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">Omschrijving:</label>"
					  "<input id=\"naam\" type=\"text\" value=\"" <<
					  scene.getOmschrijving() << "\"" << " name=\"omschrijving\"/>" << "</br>";
		ss << "</br>";
	    for (std::pair<int, FixtureFactory::Fixture*> element : scene.ff->fixturemap) {
	    	ss << "<a href=\"" << element.second->getUrl() << "\">" << element.second->naam << "</a><br>";
	    	ss << "Base adres: &nbsp;" << element.second->base_channel << "<br>";
	    	ss << "Omschrijving: &nbsp;" << element.second->omschrijving << "<br>";
	    	ss << "Channels:" << "<br>";
	    	for (int i = element.second->base_channel; i < element.second->base_channel + element.second->number_channels; i++)
	    	{
	    		ss << "<label for=\"chan" << i << "\">" << i << "</label>";
	    		ss << "<input id=\"chan" << i << "\"" <<
					  " type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"" <<
					  std::to_string(scene.channels[i-1]).c_str() << "\"" << " name=\"chan" << i << "\"/>" << "</br>";

	    	}
	    }
		ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
		ss <<  "</br>";
		ss << "<button type=\"submit\" name=\"play\" value=\"play\" id=\"play\">Play</button>";
	    ss << "</form>";
		ss <<  "</br>";
		ss << "<a href=\"/scenefactory\">Scenes</a>";
		ss <<  "</br>";
		ss << "<a href=\"/\">Home</a>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	return true;
}


