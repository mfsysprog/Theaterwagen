   //============================================================================
// Name        : Theaterwagen.cpp
// Author      : Erik Janssen
// Version     :
// Copyright   : MIT License
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <opencv2/highgui/highgui.hpp>
#include "FaceDetectorAndTracker.h"
#include "FaceSwapper.h"
#include "Schip.h"
#include "CivetServer.h"
#include "usb.h"

using namespace std;

#define DOCUMENT_ROOT "."
#define PORT "8080"

#include "uDMX_cmds.h"

#define USBDEV_SHARED_VENDOR 0x16C0  /* Obdev's free shared VID */
#define USBDEV_SHARED_PRODUCT 0x05DC /* Obdev's free shared PID */
/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */

int debug = 0;
int verbose = 0;
usb_dev_handle *handle = NULL;
int val=8,channel=1;
unsigned char values[512];
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

SDL_Texture* convertCV_MatToSDL_Texture(const cv::Mat &matImg, SDL_Renderer *ren)
{
    IplImage opencvimg2 = (IplImage)matImg;
    IplImage* opencvimg = &opencvimg2;

     //Convert to SDL_Surface
    SDL_Surface *frameSurface = SDL_CreateRGBSurfaceFrom(
                         (void*)opencvimg->imageData,
                         opencvimg->width, opencvimg->height,
                         opencvimg->depth*opencvimg->nChannels,
                         opencvimg->widthStep,
                         0xff0000, 0x00ff00, 0x0000ff, 0);

    if(frameSurface == NULL)
    {
        SDL_Log("Couldn't convert Mat to Surface.");
        return NULL;
    }

    //Convert to SDL_Texture
    SDL_Texture *frameTexture = SDL_CreateTextureFromSurface(ren, frameSurface);
    if(frameTexture == NULL)
    {
        SDL_Log("Couldn't convert Mat(converted to surface) to Texture.");
        return NULL;
    }
    else
    {
        //SDL_Log("SUCCESS conversion");
        return frameTexture;
    }
}

class AHandler : public CivetHandler
{
  private:
	bool
	handleAll(const char *method,
	          CivetServer *server,
	          struct mg_connection *conn)
	{
		std::string s[8] = "";
		std::string param = "chan";
		mg_printf(conn,
		          "HTTP/1.1 200 OK\r\nContent-Type: "
		          "text/html\r\nConnection: close\r\n\r\n");
		for (int i = 0; i < 8; i++)
		{
			CivetServer::getParam(conn, (param + std::to_string(i+1)).c_str(), s[i]);
			values[i]=atoi(s[i].c_str());
		}

		mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"1;url=/\" /></head><body>");
	    mg_printf(conn, "<h2>Setting channels using \"%s\" !</h2>", method);

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
	CivetServer server(cpp_options); // <-- C++ style start

	AHandler h_a;
	server.addHandler("/start", h_a);

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

	double fps = 0;

	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	/* SDL_WINDOW_FULLSCREEN */
	SDL_Window *win = SDL_CreateWindow("Hello World!", 0, 0, 200, 100, SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI);
	if (win == nullptr){
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	//SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);

	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr){
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	std::string imagePath = "hello.bmp";
	SDL_Surface *bmp = SDL_LoadBMP(imagePath.c_str());
	if (bmp == nullptr){
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);
	SDL_FreeSurface(bmp);
	if (tex == nullptr){
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	if (Mix_Init(SDL_INIT_AUDIO) < 0)
	    	printf("Error initializing audio: %s\n", Mix_GetError());

	    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
	    	printf("Error opening audio: %s\n", Mix_GetError());

	    // load the MP3 file "music.mp3" to play as music
	    Mix_Music *music;
	    music=Mix_LoadMUS("BennyHill.ogg");
	    if(!music) {
	        printf("Mix_LoadMUS(\"BennyHill.ogg\"): %s\n", Mix_GetError());
	        // this might be a critical error...
	    }

	    // play music forever
	    // Mix_Music *music; // I assume this has been loaded already
	    if(Mix_PlayMusic(music, -1)==-1) {
	        printf("Mix_PlayMusic: %s\n", Mix_GetError());
	        // well, there's no music, but most games don't break without music...
	    }

	    SDL_Event event;
	    bool gameRunning = true;

	    cv::UMat Uframe;

	    while (gameRunning)
	       {
	    	  if (SDL_PollEvent(&event))
	          {
	             if (event.type == SDL_QUIT)
	             {
	                gameRunning = false;
	             }

	             if (event.type == SDL_KEYDOWN)
	             {
	                switch (event.key.keysym.sym)
	                {
	                   case SDLK_ESCAPE:
	                      gameRunning = false;
	                      break;
	                }
	             }
	          }
	        try{
	        	 auto time_start = cv::getTickCount();

	        	 // Grab a frame

	        	 /*
	        	 detector >> Uframe;

	        	 //cout << "Size: " << Uframe.cols << " x " << Uframe.rows << std::endl;

	        	 cv::Mat frame = Uframe.getMat( cv::ACCESS_READ );

	        	 auto cv_faces = detector.faces();
	        	 if (cv_faces.size() == num_faces)
	        	 {
	        	    face_swapper.swapFaces(frame, cv_faces[0], cv_faces[1]);
	             }

	        	 auto time_end = cv::getTickCount();
	        	 auto time_per_frame = (time_end - time_start) / cv::getTickFrequency();

	        	 fps = (15 * fps + (1 / time_per_frame)) / 16;

	        	 printf("Total time: %3.5f | FPS: %3.2f\n", time_per_frame, fps);

	        	 // Display it all on the screen
	        	 //cv::imshow("Face Swap", frame);
	        	 tex = convertCV_MatToSDL_Texture(frame, ren); */

	        }
	        catch (exception& e)
	        {
	                cout << e.what() << endl;
	        }
	  		//First clear the renderer
	  		SDL_RenderClear(ren);
	  		//Draw the texture
	  		SDL_RenderCopy(ren, tex, NULL, NULL);
	  		//Update the screen
	  		SDL_RenderPresent(ren);
	  		//Take a quick break after all that hard work
	  		SDL_Delay(100);
	       }

    usb_close(handle);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	Mix_FreeMusic(music);
	Mix_CloseAudio();

	SDL_Quit();
	return 0;
}
