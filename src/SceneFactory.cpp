/*
x * SceneFactory.cpp
 *
 *  Created on: Mar 5, 2017
 *      Author: erik
 */

#include "SceneFactory.hpp"
#include "FixtureFactory.hpp"
#include "usb.h"
#include "uDMX_cmds.h"

#include <libintl.h>
#define _(String) gettext (String)

#define USBDEV_SHARED_VENDOR 0x16C0  /* Obdev's free shared VID */
#define USBDEV_SHARED_PRODUCT 0x05DC /* Obdev's free shared PID */
/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */

std::mutex m_scene;

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
	load();

    usb_set_debug(0);

    usb_init();
    handle = findDevice();

    if (handle == NULL) {
        fprintf(stderr,
                "Could not find USB device www.anyma.ch/uDMX (vid=0x%x pid=0x%x)\n",
                USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        //exit(1);
    }
    else uDMX_found = true;
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
SceneFactory::Scene::Scene(SceneFactory& sf, FixtureFactory* ff, std::string naam, std::string omschrijving):sf(sf){
	mh = new SceneFactory::Scene::SceneHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->channels = new std::vector<std::vector<unsigned char>>(512, std::vector<unsigned char>({0,'0','0'}));
	this->ff = ff;
	this->naam = naam;
	this->omschrijving = omschrijving;
	std::stringstream ss;
	ss << "/scene-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

SceneFactory::Scene::Scene(SceneFactory& sf, FixtureFactory* ff, std::string uuidstr, std::string naam, std::string omschrijving, unsigned int fadesteps, std::vector<std::vector<unsigned char>>* channels):sf(sf){
	mh = new SceneFactory::Scene::SceneHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->channels = channels;
	this->ff = ff;
	this->naam = naam;
	this->omschrijving = omschrijving;
	this->fadesteps = fadesteps;
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

	char filename[] = CONFIG_FILE_SCENE;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_SCENE);
	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		unsigned int fadesteps = (unsigned int) node[i]["fadesteps"].as<int>();
		std::vector<std::vector<unsigned char>>* channels = new std::vector<std::vector<unsigned char>>(512, std::vector<unsigned char>({0,'0','0'}));
		for (std::size_t k=0;k < 512;k++)
		{
			//fprintf(stderr,"Value for channel %i:  %u\n",k,atoi(node[i]["channels"][k].as<std::string>().c_str()));
			(*channels)[k][0] = atoi(node[i]["channels"][k]["value"].as<std::string>().c_str());
			(*channels)[k][1] = atoi(node[i]["channels"][k]["exclude"].as<std::string>().c_str());
			(*channels)[k][2] = atoi(node[i]["channels"][k]["selected"].as<std::string>().c_str());
		}
		SceneFactory::Scene * scene = new SceneFactory::Scene(*this, ff, uuidstr, naam, omschrijving, fadesteps, channels);
		std::string uuid_str = scene->getUuid();
		scenemap.insert(std::make_pair(uuid_str,scene));
	}
}

void SceneFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_SCENE);
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
		emitter << YAML::Key << "fadesteps";
		emitter << YAML::Value << (int) element.second->fadesteps;
		emitter << YAML::Key << "channels";
		emitter << YAML::Flow;
		emitter << YAML::BeginSeq;
		for (unsigned int i = 0; i < 512 ; i++)
		{
			emitter << YAML::BeginMap;
			emitter << YAML::Key << "value";
			emitter << std::to_string((*element.second->channels)[i][0]);
			emitter << YAML::Key << "exclude";
			emitter << std::to_string((*element.second->channels)[i][1]);
			emitter << YAML::Key << "selected";
			emitter << std::to_string((*element.second->channels)[i][2]);
			emitter << YAML::EndMap;
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

void SceneFactory::Scene::fadeIn(){
	std::thread t1( [this] {
	int nBytes;

	for (unsigned int i = 1; i <= fadesteps; ++i)
	{
	  for (int k = 0; k < 512; k++)
	  {
		  // if channel is not selected we do nothing
		  if ((*channels)[k][2] == '0') continue;
		  // exclude channel
		  if ((*channels)[k][1] == '1')
			  sf.main_channel[k] =  (*channels)[k][0];
		  else
			  sf.main_channel[k] =  (int)((float)(*channels)[k][0] / (float)fadesteps) * i;
	  }
      std::unique_lock<std::mutex> l(m_scene);
      if (sf.uDMX_found)
      {
		  nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
												cmd_SetChannelRange, 512, 0, (char *)sf.main_channel, 512, 1000);
		  if (nBytes < 0)
			 fprintf(stderr, "USB error: %s\n", usb_strerror());
      }
	  l.unlock();
	  delay(20);
	}
	Play();
	});
	t1.detach();
}

void SceneFactory::Scene::fadeOut(){
	std::thread t1( [this] {
	Play();
	int nBytes;

	for (unsigned int i = 1; i <= fadesteps; ++i)
	{
	  for (int k = 0; k < 512; k++)
	  {
		  // if channel is not selected we do nothing
		  if ((*channels)[k][2] == '0') continue;
		  // exclude channel
		  if ((*channels)[k][1] == '1')
			sf.main_channel[k] = (*channels)[k][0];
		  else
			sf.main_channel[k] = (int)(float)(*channels)[k][0] - (((float)(*channels)[k][0] / (float)fadesteps) * i);
	  }
      std::unique_lock<std::mutex> l(m_scene);
      if (sf.uDMX_found)
      {
		  nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
												cmd_SetChannelRange, 512, 0, (char *)sf.main_channel, 512, 1000);
		  if (nBytes < 0)
			 fprintf(stderr, "USB error: %s\n", usb_strerror());
      }
	  l.unlock();
	  delay(20);
	}
	});
	t1.detach();
}

void SceneFactory::Scene::Stop(){
	int nBytes;
	unsigned char channels_tmp[512] = {0};

    if (sf.uDMX_found)
    {
		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
												cmd_SetChannelRange, 512, 0, (char *)channels_tmp, 512, 1000);
		if (nBytes < 0)
			fprintf(stderr, "USB error: %s\n", usb_strerror());
    }
	delay(20);
}

void SceneFactory::Scene::Play(){
	int nBytes;

    for (int k = 0; k < 512; k++)
	  {
		  // if channel is not selected we do nothing
		  if ((*channels)[k][2] == '0') continue;
		  sf.main_channel[k] =  (*channels)[k][0];
	  }

      std::unique_lock<std::mutex> l(m_scene);
      if (sf.uDMX_found)
      {
		  nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
												cmd_SetChannelRange, 512, 0, (char *)sf.main_channel, 512, 1000);
		  if (nBytes < 0)
			 fprintf(stderr, "USB error: %s\n", usb_strerror());
      }
      l.unlock();
	  delay(20);
}

SceneFactory::Scene* SceneFactory::addScene(std::string naam, std::string omschrijving){
	SceneFactory::Scene * scene = new SceneFactory::Scene(*this, ff, naam, omschrijving);
	std::string uuid_str = scene->getUuid();
	scenemap.insert(std::make_pair(uuid_str,scene));
	return scene;
}

SceneFactory::Scene* SceneFactory::addScene(std::string naam, std::string omschrijving, std::string uuid_like){
	SceneFactory::Scene * scene = new SceneFactory::Scene(*this, ff, naam, omschrijving);
	std::string uuid_str = scene->getUuid();
	scene->fadesteps = (*scenemap.find(uuid_like)).second->getFadeSteps();
	*(scene->channels) = *((*scenemap.find(uuid_like)).second->channels);
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

unsigned int SceneFactory::Scene::getFadeSteps(){
	return fadesteps;
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
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream ss;

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/scenefactory\">";
	   this->scenefactory.deleteScene(value);
	}
	else
	/* if parameter save is present the save button was pushed */
	if(CivetServer::getParam(conn, "save", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/scenefactory\">";
		   message = _("Saved!");
        this->scenefactory.save();
	}
	else
	/* if parameter load is present the load button was pushed */
	if(CivetServer::getParam(conn, "load", dummy))
	{
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=/scenefactory\">";
		   message = _("Loaded!");
		this->scenefactory.load();
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		std::string uuid = value;
		SceneFactory::Scene* scene;

		if (!uuid.compare("-1") == 0)
		{
			CivetServer::getParam(conn, "naam", value);
			std::string naam = value;
			CivetServer::getParam(conn, "omschrijving", value);
			std::string omschrijving = value;

			scene = scenefactory.addScene(naam, omschrijving, uuid);
		}
		else
		{
			CivetServer::getParam(conn, "naam", value);
			std::string naam = value;
			CivetServer::getParam(conn, "omschrijving", value);
			std::string omschrijving = value;

			scene = scenefactory.addScene(naam, omschrijving);
		}

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + scene->getUrl() + "\"/>";
	}

	if(CivetServer::getParam(conn, "new", value))
	{
	   ss << "<form action=\"/scenefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << ":</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"10\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"20\" name=\"omschrijving\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"" << value << "\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";
	}
	else
	/* initial page display */
	{
		std::map<std::string, SceneFactory::Scene*>::iterator it = scenefactory.scenemap.begin();
		for (std::pair<std::string, SceneFactory::Scene*> element : scenefactory.scenemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << "&nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving();
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.second->getUuid() << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" value=\"-1\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	}

	ss = getHtml(meta, message, "scene",  ss.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}

bool SceneFactory::Scene::SceneHandler::handleAll(const char *method,
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
		scene.naam = value.c_str();
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
		scene.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "fadesteps", value))
	{
		CivetServer::getParam(conn,"value", value);
		scene.fadesteps = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "chan", value))
	{
		int channel = atoi(value.c_str());
		CivetServer::getParam(conn,"value", value);
		(*scene.channels)[channel-1][0] = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		scene.Play();
		return true;
	}
	if(CivetServer::getParam(conn, "exclude", value))
	{
		int channel = atoi(value.c_str());
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			(*scene.channels)[channel-1][1] = '1';
		else
			(*scene.channels)[channel-1][1] = '0';
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "selected", value))
	{
		int channel = atoi(value.c_str());
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			(*scene.channels)[channel-1][2] = '1';
		else
			(*scene.channels)[channel-1][2] = '0';
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "play", dummy))
	{
		scene.Play();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + scene.getUrl() + "\"/>";
		message = _("Playing!");
	}
	if(CivetServer::getParam(conn, "fadein", dummy))
	{
		scene.fadeIn();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + scene.getUrl() + "\"/>";
		message = _("Fading in!");
	}
	if(CivetServer::getParam(conn, "fadeout", dummy))
	{
		scene.fadeOut();
		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + scene.getUrl() + "\"/>";
		message = _("Fading out!");
	}

	/* initial page display */
	{
		ss << "<form action=\"" << scene.getUrl() << "\" method=\"POST\">";
		ss << "<button type=\"submit\" name=\"refresh\" value=\"true\" id=\"refresh\">" << _("Refresh") << "</button><br>";
		ss << "<br>";
		ss << "<button type=\"submit\" name=\"play\" value=\"play\" id=\"play\">" << _("Play") << "</button>";
		ss << "<button type=\"submit\" name=\"fadein\" value=\"fadein\" id=\"fadein\">" << _("Fade In") << "</button>";
		ss << "<button type=\"submit\" name=\"fadeout\" value=\"fadeout\" id=\"fadeout\">" << _("Fade Out") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/scenefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" value=\"" << scene.getUuid() <<"\" id=\"new\">" << _("New Like") << "</button>";
	    ss << "</form>";
	    ss << "<div style=\"clear:both\">";
	    ss << "<h2>";
	    ss << "&nbsp;";
	    ss << "</h2>";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" value=\"" <<
					  scene.getNaam() << "\"" << " name=\"naam\"/>" << "</br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" value=\"" <<
					  scene.getOmschrijving() << "\"" << " name=\"omschrijving\"/>" << "</br>";
		ss << "<br>";
		ss << "<label for=\"fadesteps\">" << _("Fade Steps") << ":</label>";
		ss << "<td><input class=\"inside\" id=\"fadesteps\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			  scene.getFadeSteps() << "\"" << " name=\"fadesteps\" />";
		ss << "</tr>";
		ss << "</div>";
		ss << "<br>";
		ss << "<br>";
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << scene.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << scene.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#fadesteps').on('change', function() {";
		tohead << " $.get( \"" << scene.getUrl() << "\", { fadesteps: 'true', value: $('#fadesteps').val() }, function( data ) {";
		tohead << "  $( \"#fadesteps\" ).html( data );})";
	    tohead << "});";


	    for (std::pair<int, FixtureFactory::Fixture*> element : scene.ff->fixturemap) {
	    	ss << "<a href=\"" << element.second->getUrl() << "\">" << element.second->naam << "</a><br>";
	    	ss << _("Base address") << ": &nbsp;" << element.second->base_channel << "<br>";
	    	ss << _("Comment") << ": &nbsp;" << element.second->omschrijving << "<br>";
	    	ss << _("Channels") << ":<br>";
	    	ss << "<table class=\"container\"><th>" << _("Channel") << "</th><th>" << _("Selected") << "</><th>" << _("Exclude from Fade") << "</th><th>" << _("Value") << "</th>";
	    	for (int i = element.second->base_channel; i < element.second->base_channel + element.second->number_channels; i++)
	    	{
	    		tohead << " $('#chan" << i <<"').on('change', function() {";
	    		tohead << " $.get( \"" << scene.getUrl() << "\", { chan: \"" << i << "\", value: $('#chan" << i << "').val() }, function( data ) {";
	    		tohead << "  $( \"#chan" << i << "\" ).html( data );})";
      		    tohead << "});";
	    		tohead << " $('#exclude" << i <<"').on('change', function() {";
	    		tohead << " $.get( \"" << scene.getUrl() << "\", { exclude: \"" << i << "\", value: $('#exclude" << i << "').is(':checked') }, function( data ) {";
	    		tohead << "  $( \"#exclude" << i << "\" ).html( data );})";
      		    tohead << "});";
	    		tohead << " $('#selected" << i <<"').on('change', function() {";
	    		tohead << " $.get( \"" << scene.getUrl() << "\", { selected: \"" << i << "\", value: $('#selected" << i << "').is(':checked') }, function( data ) {";
	    		tohead << "  $( \"#selected" << i << "\" ).html( data );})";
      		    tohead << "});";

	    		ss << "<tr>";
	    		ss << "<td style=\"text-align:center;\"><label for=\"chan" << i << "\">" << i << "</label></td>";
	    		if ((*scene.channels)[i-1][2] == '1')
	    			ss << "<td style=\"text-align:center;\"><input id=\"selected" << i << "\"" <<
						" type=\"checkbox\" value=\"ja\" name=\"selected" << i << "\" checked/>" << "</td>";
	    		else
	    			ss << "<td style=\"text-align:center;\"><input id=\"selected" << i << "\"" <<
						" type=\"checkbox\" value=\"ja\" name=\"selected" << i << "\"/>" << "</td>";
	    		if ((*scene.channels)[i-1][1] == '1')
	    			ss << "<td style=\"text-align:center;\"><input id=\"exclude" << i << "\"" <<
						" type=\"checkbox\" value=\"ja\" name=\"exclude" << i << "\" checked/>" << "</td>";
	    		else
	    			ss << "<td style=\"text-align:center;\"><input id=\"exclude" << i << "\"" <<
						" type=\"checkbox\" value=\"ja\" name=\"exclude" << i << "\"/>" << "</td>";
	    		ss << "<td><input id=\"chan" << i << "\"" <<
					  " type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"" <<
					  std::to_string((*scene.channels)[i-1][0]).c_str() << "\"" << " name=\"chan" << i << "\"/>" << "</td>";
	    		ss << "</tr>";
	    	}
		    ss << "</table>";
	    }
	    tohead << "});";
		tohead << "</script>";

	    ss << "<br>";
	}

	ss = getHtml(meta, message, "scene", ss.str().c_str(), tohead.str().c_str());
	mg_printf(conn,  ss.str().c_str(), "%s");
	return true;
}



