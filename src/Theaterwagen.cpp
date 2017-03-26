   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "FaceDetectorAndTracker.h"
#include "FaceSwapper.h"
#include "Schip.h"
#include "CivetServer.h"
#include "usb.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "UploadHandler.hpp"
#include "MusicFactory.hpp"

using namespace std;

#define DOCUMENT_ROOT "."
#define PORT "8080"

#include "uDMX_cmds.h"

#define USBDEV_SHARED_VENDOR 0x16C0  /* Obdev's free shared VID */
#define USBDEV_SHARED_PRODUCT 0x05DC /* Obdev's free shared PID */
/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */


CivetServer* server = nullptr;
int debug = 0;
int verbose = 0;
usb_dev_handle *handle = NULL;
int val=8,channel=1;
unsigned char values[512] = {0};
int nBytes;


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

class FixtureHandler : public CivetHandler
{
  private:
	bool
	handleAll(const char *method,
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
		   for (int i = 0; i < 8; i++)
		   {
			 CivetServer::getParam(conn, (param + std::to_string(i+1)).c_str(), s[i]);
			 values[i]=atoi(s[i].c_str());
		   }
		   mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/fixture1\" /></head><body>");
		   mg_printf(conn, "<h2>Changes committed...!</h2>");
		}
		else
		{
			mg_printf(conn, "<html><body>");
			mg_printf(conn, "<form action=\"/fixture1\" method=\"get\">");
			mg_printf(conn, "<input type=\"hidden\" name=\"submit\" value=\"true\">");
			for (int i = 0; i < 8; i++)
			{
			   std::stringstream ss;
			   const char* channel = (std::to_string(i+1)).c_str();
			   const char* value = (std::to_string(values[i])).c_str();
			   ss << "<label for=\"chan" << channel << "\">Channel" << channel << "</label>"
					 "<input id=\"chan" << channel << "\"" <<
					 " type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"" <<
					 value << "\"" << " name=\"chan" << channel << "\"/>" << "</br>";
		       mg_printf(conn, ss.str().c_str());
			}
			mg_printf(conn, "<input type=\"submit\" value=\"Submit\" id=\"submit\" />");
			mg_printf(conn, "<a href=\"/\">Home</a>");
			mg_printf(conn, "</body></html>");
		}

		nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
			                                    cmd_SetChannelRange, val, channel - 1, (char *)values, val, 1000);
		if (nBytes < 0)
		   fprintf(stderr, "USB error: %s\n", usb_strerror());
		mg_printf(conn, "</body></html>\n");
		return true;
	}

  public:
	bool
	handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}
	bool
	handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}
};

/*
 * Lesson 0: Test to make sure SDL is setup properly
 */
int main(int, char**){

	const char *options[] = {
		    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0};

	    std::vector<std::string> cpp_options;
	    for (int i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
	        cpp_options.push_back(options[i]);
	    }

    // CivetServer server(options); // <-- C style start
	CivetServer server_start(cpp_options); // <-- C++ style start
	server = &server_start;

	FixtureHandler h_a[2];
	server->addHandler("/fixture1", h_a[0]);

	UploadHandler hu;
	server->addHandler("/upload", hu);

    usb_set_debug(0);

    usb_init();
    handle = findDevice();
    if (handle == NULL) {
        fprintf(stderr,
                "Could not find USB device www.anyma.ch/uDMX (vid=0x%x pid=0x%x)\n",
                USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        exit(1);
    }

	const size_t num_faces = 2;
	//FaceDetectorAndTracker detector("haarcascade_frontalface_default.xml", 0, num_faces);
	//FaceSwapper face_swapper("shape_predictor_68_face_landmarks.dat");

	if (wiringPiSetupGpio() != 0){
		std::cerr << "Unable to start wiringPi interface!" << std::endl;
		return 1;
	}
	Schip s1(27,22,17,18);
	server->addHandler("/schip", s1.getHandler());

	MusicFactory m1;

	/*
	while(!(s1.Full(LEFT)))
	{
		delay(100);
	}
	s1.Start(RIGHT);

	while(!(s1.Full(RIGHT)))
		{
			delay(100);
		}
	s1.Start(LEFT);
	*/

	double fps = 0;

    while (1==1){
    	delay(100);
    }

    usb_close(handle);

	return 0;
}
