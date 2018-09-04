
/*
 * CloneFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "CloneFactory.hpp"
using namespace std;
using namespace cv;
using namespace sf;
using namespace sfe;

#include <chrono>
#include <iostream>
using namespace std::chrono;

#include <libintl.h>
#define _(String) gettext (String)

std::mutex m;

bool first_open = true;

class mythread : public std::thread
{
  public:
    mythread() {}
    static void setScheduling(std::thread &th, int policy, int priority) {
        sched_param sch_params;
        sch_params.sched_priority = priority;
        if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
        	(*syslog) << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
        }
    }
  private:
    sched_param sch_params;
};

static std::vector<cv::Rect> boundRect(Mat* mask, vector<vector<Point> >* contours, vector<Vec4i>* hierarchy)
{
	  findContours( *mask, *contours, *hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, Point(0, 0) );

	    std::vector<std::vector<cv::Point>> contours_poly( contours->size() );
	    std::vector<cv::Rect> rect( contours->size() );
	    for( unsigned int i = 0; i < contours->size(); i++ )
	        {
	            approxPolyDP( Mat((*contours)[i]), contours_poly[i], 3, true );
	            rect[i] = boundingRect( Mat(contours_poly[i]) );
	        }
	    return rect;
}

static std::string matToJPG(cv::Mat* input)
{
    std::vector<uchar> buf;
    cv::Mat resized;
    try
    {
        cv::resize((*input), resized, cv::Size(VIDEO_WIDTH,VIDEO_HEIGHT), 0, 0);
        int params[2] = {0};
        params[0] = cv::IMWRITE_JPEG_QUALITY;
        params[1] = 100;
    	cv::imencode(".jpg", resized, buf, std::vector<int>(params, params+1) );
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        (*syslog) << "exception caught: " << err_msg << std::endl;
    }

    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    return incoming.str();
}

static cv::Mat ImgToMat(std::string* input)
{

	std::vector<uchar> buf(input->begin(),input->end());

    cv::Mat restored;
    try
    {
        cv::imdecode(buf, cv::IMREAD_UNCHANGED, &restored);
        //cv::cvtColor(restored, restored, CV_RGB2BGR);
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        (*syslog) << "exception caught: " << err_msg << std::endl;
    }

    return restored;
}

/*
static cv::UMat ImgToUMat(std::string* input)
{
	cv::UMat restored;
	ImgToMat(input).copyTo(restored);
    return restored;
}*/

static cv::Mat drawEllipses(cv::Mat* input, std::vector<ellipse_s>* ellipses)
{
	cv::Mat output = (*input).clone();
	//Read points
    for (unsigned i = 0; i < (*ellipses).size(); ++i)
    {
        ellipse( output, Point( (*ellipses)[i].centerx, (*ellipses)[i].centery),
        		         Size( (*ellipses)[i].axeheight, (*ellipses)[i].axewidth ),
						 (*ellipses)[i].angle, 0, 360,
						 Scalar( (*ellipses)[i].blue, (*ellipses)[i].green, (*ellipses)[i].red),
						 (*ellipses)[i].thickness, (*ellipses)[i].linetype );
    }
    return output;
}

/*
 * CloneFactory Constructor en Destructor
 */
CloneFactory::CloneFactory(){
    mfh = new CloneFactory::CloneFactoryHandler(*this);
	server->addHandler("/clonefactory", mfh);
	camMat = new cv::Mat();
	load();
	std::thread t1( [this] {
		setenv("DISPLAY",":0.0",0);
//		this->window = new sf::RenderWindow(sf::VideoMode(1024, 768), "RenderWindow",sf::Style::Fullscreen);
		//this->window = new sf::RenderWindow(sf::VideoMode(1024, 768), "RenderWindow");
		//sf::RenderWindow window(sf::VideoMode(640, 480), "RenderWindow");
		sf::VideoMode desktop = sf::VideoMode().getDesktopMode();
		this->window = new sf::RenderWindow(desktop, "Theaterwagen", sf::Style::None);
	    window->setMouseCursorVisible(false);
	    window->setVerticalSyncEnabled(true);
	    //window->setFramerateLimit(10);
	    //window->setActive(false);
		renderingThread(window); } );
	//mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();
}

CloneFactory::~CloneFactory(){
	delete mfh;
	delete camMat;
	std::map<std::string, CloneFactory::Clone*>::iterator it = clonemap.begin();
	if (it != clonemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    clonemap.erase(it);
	}
}

/*
 * CloneFactoryHandler Constructor en Destructor
 */
CloneFactory::CloneFactoryHandler::CloneFactoryHandler(CloneFactory& clonefactory):clonefactory(clonefactory){
}


CloneFactory::CloneFactoryHandler::~CloneFactoryHandler(){
}

/*
 * Clone Constructor en Destructor
 */
CloneFactory::Clone::Clone(CloneFactory& cf, std::string naam, std::string omschrijving):cf(cf){
	mh = new CloneFactory::Clone::CloneHandler(*this);
	uuid_generate( (unsigned char *)&uuid );

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->cap = new cv::VideoCapture();

	ellipses_in = new std::vector<ellipse_s>();
	ellipses_out = new std::vector<ellipse_s>();

	//cv::Mat boodschap(1024,768,CV_8UC3,cv::Scalar(255,255,255));
	//cv::putText(boodschap, "Gezichtsherkenningsmodel wordt geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	//std::unique_lock<std::mutex> l(m);
	//this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
	//l.unlock();
	//detector = new dlib::frontal_face_detector();

	std::stringstream ss;
	ss << "/clone-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CloneFactory::Clone::Clone(CloneFactory& cf, std::string uuidstr, std::string naam,
		                         std::string omschrijving, std::string filename, std::string filename2,
								 std::vector<ellipse_s>* ellipses_in,
								 std::vector<ellipse_s>* ellipses_out,
								 bool fileonly,
								 bool clone_to_file,
								 bool clone_to_file2,
								 unsigned int mix_file,
								 unsigned int mix_file2,
								 unsigned int filesteps,
								 unsigned int file2steps,
								 unsigned int morphsteps):cf(cf){
	mh = new CloneFactory::Clone::CloneHandler(*this);
	uuid_parse(uuidstr.c_str(), (unsigned char *)&uuid);

	this->naam = naam;
	this->omschrijving = omschrijving;
	this->cap = new cv::VideoCapture();
	this->filename = filename;
	this->filename2 = filename2;
	this->ellipses_in = ellipses_in;
	this->ellipses_out = ellipses_out;
	this->fileonly = fileonly;
	this->clone_to_file = clone_to_file;
	this->clone_to_file2 = clone_to_file2;
	this->mix_file = mix_file;
	this->mix_file2 = mix_file2;
	this->filesteps = filesteps;
	this->file2steps = file2steps;
	this->morphsteps = morphsteps;

	//cv::Mat boodschap(1024,768,CV_8UC3,cv::Scalar(255,255,255));
	//cv::putText(boodschap, "Gezichtsherkenningsmodel wordt geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	//std::unique_lock<std::mutex> l(m);
	//this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
	//l.unlock();
	//detector = new dlib::frontal_face_detector();

	std::stringstream ss;
	ss << "/clone-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CloneFactory::Clone::~Clone(){
	delete mh;
	delete cap;
	delete ellipses_in;
	delete ellipses_out;
	//delete detector;
}

/*
 * Clone Handler Constructor en Destructor
 */
CloneFactory::Clone::CloneHandler::CloneHandler(CloneFactory::Clone& clone):clone(clone){
}

CloneFactory::Clone::CloneHandler::~CloneHandler(){
}


/* overige functies
 *
 */

void CloneFactory::renderingThread(sf::RenderWindow *window)
{
	sf::Image image;
	sf::Texture texture;
	sf::Sprite sprite;
	sfe::Movie movie;

	// the rendering loop
    while (window->isOpen())
    {
    	if (loadme)
    	{
    		try
		    {
    			if (movie.openFromFile(on_screen))
    			{
    				movie.play();
    				loaded = true;
    				loadme = false;
    			}
    			else
    			{
    				(*syslog) << "Videofile kon niet geopend worden!" << endl;
    			}
    		}
    		catch( cv::Exception& e )
    		{
    		  	(*syslog) << "Error loading merged mp4: " << e.msg << endl;
    		}

    	}
		window->clear(sf::Color::Black);
    	if (loaded)
    	{
        	if (movie.getStatus() == sfe::Status::Stopped)
        		movie.play();
			movie.update();
			movie.fit(0,0,window->getSize().x,window->getSize().y, false);
	   		window->draw(movie);
    	}
 		window->display();

		sf::Event event;
        while (window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window->close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    *(syslog) << "the escape key was pressed" << std::endl;
                    window->close();
                }
            }
        }
    }
}


void CloneFactory::load(){
	for (std::pair<std::string, CloneFactory::Clone*> element  : clonemap)
	{
		deleteClone(element.first);
	}

	char filename[] = CONFIG_FILE_CLONE;
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

	YAML::Node node = YAML::LoadFile(CONFIG_FILE_CLONE);

	for (std::size_t i=0;i<node.size();i++) {
		std::string uuidstr = node[i]["uuid"].as<std::string>();
		std::string naam = node[i]["naam"].as<std::string>();
		std::string omschrijving = node[i]["omschrijving"].as<std::string>();
		std::string filename = node[i]["filename"].as<std::string>();
		std::string filename2 = node[i]["filename2"].as<std::string>();
		bool fileonly = node[i]["fileonly"].as<bool>();
		bool clone_to_file = node[i]["clone_to_file"].as<bool>();
		bool clone_to_file2 = node[i]["clone_to_file2"].as<bool>();
		unsigned int mix_file = (unsigned int) node[i]["mix_file"].as<int>();
		unsigned int mix_file2 = (unsigned int) node[i]["mix_file2"].as<int>();
		unsigned int filesteps = (unsigned int) node[i]["filesteps"].as<int>();
		unsigned int file2steps = (unsigned int) node[i]["file2steps"].as<int>();
		unsigned int morphsteps = (unsigned int) node[i]["morphsteps"].as<int>();
		std::vector<ellipse_s>* ellipses_in = new std::vector<ellipse_s>();
		for (std::size_t ellipse=0;ellipse < node[i]["ellipses_in"].size();ellipse++)
		{
			ellipse_s elli;
			elli.centerx = node[i]["ellipses_in"][ellipse]["centerx"].as<int>();
			elli.centery = node[i]["ellipses_in"][ellipse]["centery"].as<int>();
			elli.axeheight = node[i]["ellipses_in"][ellipse]["axeheight"].as<float>();
			elli.axewidth = node[i]["ellipses_in"][ellipse]["axewidth"].as<float>();
			elli.angle = node[i]["ellipses_in"][ellipse]["angle"].as<double>();
			elli.blue = node[i]["ellipses_in"][ellipse]["blue"].as<double>();
			elli.green = node[i]["ellipses_in"][ellipse]["green"].as<double>();
			elli.red = node[i]["ellipses_in"][ellipse]["red"].as<double>();
			elli.thickness = node[i]["ellipses_in"][ellipse]["thickness"].as<int>();
			elli.linetype = node[i]["ellipses_in"][ellipse]["linetype"].as<int>();
			ellipses_in->push_back(elli);
		}
		std::vector<ellipse_s>* ellipses_out = new std::vector<ellipse_s>();
		for (std::size_t ellipse=0;ellipse < node[i]["ellipses_out"].size();ellipse++)
		{
			ellipse_s elli;
			elli.centerx = node[i]["ellipses_out"][ellipse]["centerx"].as<int>();
			elli.centery = node[i]["ellipses_out"][ellipse]["centery"].as<int>();
			elli.axeheight = node[i]["ellipses_out"][ellipse]["axeheight"].as<float>();
			elli.axewidth = node[i]["ellipses_out"][ellipse]["axewidth"].as<float>();
			elli.angle = node[i]["ellipses_out"][ellipse]["angle"].as<double>();
			elli.blue = node[i]["ellipses_out"][ellipse]["blue"].as<double>();
			elli.green = node[i]["ellipses_out"][ellipse]["green"].as<double>();
			elli.red = node[i]["ellipses_out"][ellipse]["red"].as<double>();
			elli.thickness = node[i]["ellipses_out"][ellipse]["thickness"].as<int>();
			elli.linetype = node[i]["ellipses_out"][ellipse]["linetype"].as<int>();
			ellipses_out->push_back(elli);
		}
		CloneFactory::Clone * clone =
			new CloneFactory::Clone(*this,
					uuidstr, naam, omschrijving,
					filename, filename2, ellipses_in, ellipses_out,
					fileonly, clone_to_file, clone_to_file2,
					mix_file, mix_file2, filesteps, file2steps, morphsteps);
		std::string uuid_str = clone->getUuid();
		clonemap.insert(std::make_pair(uuid_str,clone));
	}
}

void CloneFactory::save(){
	YAML::Emitter emitter;
	std::ofstream fout(CONFIG_FILE_CLONE);
	std::map<std::string, CloneFactory::Clone*>::iterator it = clonemap.begin();

	emitter << YAML::BeginSeq;

	for (std::pair<std::string, CloneFactory::Clone*> element  : clonemap)
	{
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "uuid";
		emitter << YAML::Value << element.first;
		emitter << YAML::Key << "naam";
		emitter << YAML::Value << element.second->naam;
		emitter << YAML::Key << "omschrijving";
		emitter << YAML::Value << element.second->omschrijving;
		emitter << YAML::Key << "filename";
		emitter << YAML::Value << element.second->filename;
		emitter << YAML::Key << "filename2";
		emitter << YAML::Value << element.second->filename2;
		emitter << YAML::Key << "fileonly";
		emitter << YAML::Value << element.second->fileonly;
		emitter << YAML::Key << "clone_to_file";
		emitter << YAML::Value << element.second->clone_to_file;
		emitter << YAML::Key << "clone_to_file2";
		emitter << YAML::Value << element.second->clone_to_file2;
		emitter << YAML::Key << "mix_file";
    	emitter << YAML::Value << (int) element.second->mix_file;
    	emitter << YAML::Key << "mix_file2";
		emitter << YAML::Value << (int) element.second->mix_file2;
    	emitter << YAML::Key << "filesteps";
		emitter << YAML::Value << (int) element.second->filesteps;
    	emitter << YAML::Key << "file2steps";
		emitter << YAML::Value << (int) element.second->file2steps;
    	emitter << YAML::Key << "morphsteps";
		emitter << YAML::Value << (int) element.second->morphsteps;
		emitter << YAML::Key << "ellipses_in";
		emitter << YAML::BeginSeq;
        /* ellipses input */
	    for (unsigned int ellipse = 0; ellipse < (*element.second->ellipses_in).size() ; ellipse++)
		{
			  emitter << YAML::BeginMap;
			  emitter << YAML::Key << "centerx";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].centerx;
			  emitter << YAML::Key << "centery";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].centery;
			  emitter << YAML::Key << "axeheight";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].axeheight;
			  emitter << YAML::Key << "axewidth";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].axewidth;
			  emitter << YAML::Key << "angle";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].angle;
			  emitter << YAML::Key << "blue";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].blue;
			  emitter << YAML::Key << "green";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].green;
			  emitter << YAML::Key << "red";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].red;
			  emitter << YAML::Key << "thickness";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].thickness;
			  emitter << YAML::Key << "linetype";
			  emitter << YAML::Value << (*element.second->ellipses_in)[ellipse].linetype;
			  emitter << YAML::EndMap;
		}
		emitter << YAML::EndSeq;
		emitter << YAML::Key << "ellipses_out";
		emitter << YAML::BeginSeq;
        /* ellipses input */
	    for (unsigned int ellipse = 0; ellipse < (*element.second->ellipses_out).size() ; ellipse++)
		{
			  emitter << YAML::BeginMap;
			  emitter << YAML::Key << "centerx";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].centerx;
			  emitter << YAML::Key << "centery";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].centery;
			  emitter << YAML::Key << "axeheight";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].axeheight;
			  emitter << YAML::Key << "axewidth";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].axewidth;
			  emitter << YAML::Key << "angle";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].angle;
			  emitter << YAML::Key << "blue";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].blue;
			  emitter << YAML::Key << "green";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].green;
			  emitter << YAML::Key << "red";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].red;
			  emitter << YAML::Key << "thickness";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].thickness;
			  emitter << YAML::Key << "linetype";
			  emitter << YAML::Value << (*element.second->ellipses_out)[ellipse].linetype;
			  emitter << YAML::EndMap;
		}
		emitter << YAML::EndSeq;
		emitter << YAML::EndMap;
	}
	emitter << YAML::EndSeq;
	fout << emitter.c_str();
}

void CloneFactory::Clone::openCap(cloneType type)
{
	if (type == CAP_CAM)
	{
		if(!cap->isOpened()){
			cap->open(CAP_GPHOTO2); // connect to the camera
			if(!cap->isOpened())
				{
					if (first_open)
					{
						(*syslog) << "Could not open SLR camera, retrying with webcam." << endl;
						first_open = false;
					}
					cap->open(0); //if no camera, try webcam on usb0
				}
			// if camera found output all of its options
			else
			if (first_open)
			{
				(*syslog) << (const char*)((intptr_t)cap->get(cv::CAP_PROP_GPHOTO2_WIDGET_ENUMERATE)) << endl;
				first_open = false;
			}
			cap->set(cv::CAP_PROP_FOURCC , cv::VideoWriter::fourcc('M','J','P','G') );
			cap->set(cv::CAP_PROP_FRAME_WIDTH,2304);   // width pixels 2304
			cap->set(cv::CAP_PROP_FRAME_HEIGHT,1296);   // height pixels 1296
		}
	}
	else if (type == CAP_FILE)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filename);
		}
	}
	else if (type == CAP_FILE2)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filename2);
		}
	}
}

void CloneFactory::Clone::closeCap(){
	if(cap->isOpened()){   // connect to the camera
		cap->release();
    }

}

void CloneFactory::Clone::mergeToScreen()
{
	std::unique_lock<std::mutex> l(m);

	if (!fileonly)
	{
		mergeFrames();
	}

	l.unlock();
	return;
}

void CloneFactory::Clone::mergeToFile()
{
	std::unique_lock<std::mutex> l(m);

	if (fileonly)
	{
		mergeFrames();
	}

	l.unlock();
	return;
}

void CloneFactory::Clone::loadFilmpje()
{
    openCap(CAP_FILE);
	cv::Mat frame;
	std::vector<std::string> jpg;
	for (;;)
	{
	   frame = cloneFrame(CAP_FILE);
	   if (frame.empty()) break;
	    jpg.push_back(matToJPG(&frame));
	}
	closeCap();
}

void CloneFactory::clearScreen()
{
	 loaded = false;
	 loadme = false;
}

void CloneFactory::Clone::onScreen()
{
	 delay(200);
	 std::unique_lock<std::mutex> l(m);
	 cf.on_screen = TMP_DIR + this->getUuid() + "." + VIDEO_EXT;
	 cf.loaded = false;
	 cf.loadme = true;
	 l.unlock();
	 /*
	 delay(200);
	 std::unique_lock<std::mutex> l3(m_merging);
	 std::unique_lock<std::mutex> l(m_screen);
	 std::unique_lock<std::mutex> l2(m);
	 delete cf.on_screen;
	 cf.on_screen = new std::vector<std::vector<char>>();
	 for (unsigned int i = 0; i < off_screen->size(); ++i)
	 {
		 const char *cstr = (*off_screen)[i].str().c_str();
		 std::vector<char> tmp(cstr, cstr + (*off_screen)[i].str().size());
		 cf.on_screen->push_back(tmp);
	 }
	 this->off_screen = new std::vector<std::stringstream>();
	 l.unlock();
	 l2.unlock();
	 l3.unlock();
	 */
}

void CloneFactory::Clone::getFrame(int framenumber, bool draw){
	std::string photo = TMP_DIR + this->getUuid() + "_file.jpg";
	std::vector<int> compression_params;
	compression_params.push_back(IMWRITE_JPEG_QUALITY);
	compression_params.push_back(95);
    std::unique_lock<std::mutex> l(m);
	openCap(CAP_FILE);
	this->cap->set(CAP_PROP_POS_FRAMES, framenumber - 1);
	cv::Mat input;
	*cap >> input;
	closeCap();
	if (draw)
		input = drawEllipses(&input,ellipses_out);
	imwrite(photo.c_str(), input, compression_params);
	l.unlock();
}

cv::Mat CloneFactory::Clone::cloneFrame(cloneType clonetype){
    // Grab a frame
	cv::Mat input;
	try
	{
		*cap >> input;
		if (!input.empty())
		{
			cv::resize(input,input,cv::Size(VIDEO_WIDTH,VIDEO_HEIGHT));
		}
		if (clonetype == CAP_CAM)
		{
			std::string photo = TMP_DIR + this->getUuid() + "_photo.jpg";
			std::vector<int> compression_params;
			compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
			compression_params.push_back(95);
			input = drawEllipses(&input, this->ellipses_in);
			imwrite(photo.c_str(), input, compression_params);
		}
	}
	catch( cv::Exception& e )
	{
	    const char* err_msg = e.what();
	    (*syslog) << "exception caught: " << err_msg << std::endl;
	}
	return input;
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */
}


std::vector<std::string> CloneFactory::Clone::morph(cv::Mat orig, cv::Mat target, unsigned int morphsteps)
{
      std::vector<std::string> totaal;

      cv::Mat resultaat;
	  unsigned int step = (unsigned int) 100 / morphsteps;

	  for (unsigned int i = 0; i < morphsteps; i++)
	  {
		  float value = ((float)step * (float)i) / 100.0;
		  addWeighted(orig, 1.0 - value, target, value, 0.0, resultaat);
		  totaal.push_back(matToJPG(&resultaat));
	  }

	  return totaal;
}

std::vector<std::string> CloneFactory::Clone::copyEllipses(cloneType type, unsigned int mix_file, std::vector<ellipse_s>* ellipses_in, std::vector<ellipse_s>* ellipses_out, bool fileonly)
{
	std::vector<std::string> totaal;

	openCap(type);

	cv::Mat img_file;

	/*
	 * if we have no ellipses in the output we just copy it asis
	*/
	if ((*ellipses_out).size() == 0) {
		while(true)
		{
			img_file = cloneFrame(type);
			if (img_file.empty()) break;

			totaal.push_back(matToJPG(&img_file));
		}
		closeCap();
		return totaal;
	}
	else
	//if we have no ellipses in the input we just copy it asis
	if ((*ellipses_in).size() == 0) {
		while(true)
		{
			img_file = cloneFrame(type);
			if (img_file.empty()) break;

			totaal.push_back(matToJPG(&img_file));
		}
		closeCap();
		return totaal;
	}
	else
	while (true)
	{
		img_file = cloneFrame(type);
		if (img_file.empty()) break;
		if ((type == CAP_FILE && clone_to_file) || (type == CAP_FILE2 && clone_to_file2))
		{
      		std::vector<std::vector<cv::Point> > src_contours, tgt_contours;
      		std::vector<cv::Vec4i> src_hierarchy, tgt_hierarchy;
      		cv::Mat src_gray, tgt_gray;

      		cvtColor( (*cf.camMat), src_gray, cv::COLOR_BGR2GRAY );
      		cvtColor( img_file, tgt_gray, cv::COLOR_BGR2GRAY );

			Mat src_mask(src_gray.rows, src_gray.cols, CV_8UC1, Scalar(0,0,0));
			Mat tgt_mask(tgt_gray.rows, tgt_gray.cols, CV_8UC1, Scalar(0,0,0));

			for (unsigned int i = 0; i < ellipses_in->size(); i++)
			{
				ellipse( src_mask, Point( (*ellipses_in)[i].centerx, (*ellipses_in)[i].centery ),
						 Size( (*ellipses_in)[i].axewidth, (*ellipses_in)[i].axeheight ),
						 0,0,
						 (*ellipses_in)[i].angle,
						 Scalar( (*ellipses_in)[i].red,
								 (*ellipses_in)[i].green,
								 (*ellipses_in)[i].blue ),
						(*ellipses_in)[i].thickness,
						(*ellipses_in)[i].linetype);
			}

			for (unsigned int i = 0; i < ellipses_out->size(); i++)
			{
				ellipse( tgt_mask, Point( (*ellipses_out)[i].centerx, (*ellipses_out)[i].centery ),
						 Size( (*ellipses_out)[i].axewidth, (*ellipses_out)[i].axeheight ),
						 0,0,
						 (*ellipses_out)[i].angle,
						 Scalar( (*ellipses_out)[i].red,
								 (*ellipses_out)[i].green,
								 (*ellipses_out)[i].blue ),
						(*ellipses_out)[i].thickness,
						(*ellipses_out)[i].linetype);
			}

			std::vector<cv::Rect> srcRect, tgtRect;
			srcRect = boundRect(&src_mask, &src_contours, &src_hierarchy);
			tgtRect = boundRect(&tgt_mask, &tgt_contours, &tgt_hierarchy);

			//Mat drawing = Mat::zeros( src_mask.size(), CV_8UC3 );
			cvtColor(src_mask,src_mask,cv::COLOR_GRAY2RGB);
			cvtColor(tgt_mask,tgt_mask,cv::COLOR_GRAY2RGB);
			for( unsigned int i = 0; i < srcRect.size(); i++ )
			{
			   Mat src_resized;
			   resize((*cf.camMat)(srcRect[i]), src_resized, Size(tgtRect[i].width, tgtRect[i].height));
			   src_resized.copyTo(img_file(tgtRect[i]),tgt_mask(tgtRect[i]));
			}
		}
		totaal.push_back(matToJPG(&img_file));
	}
	closeCap();
	return totaal;
}

void CloneFactory::Clone::captureAndMerge()
{
	openCap(CAP_CAM);
	std::unique_lock<std::mutex> l(m);
	cf.camMat->release();
	(*cf.camMat) = cloneFrame(CAP_CAM);

	closeCap();

	mergeFrames();

	l.unlock();
	return;
}

void CloneFactory::Clone::mergeFrames()
{

	std::vector<std::string> totaal;
	std::vector<std::string> file;
	std::vector<std::string> file2;
	std::vector<std::string> morph;

	high_resolution_clock::time_point t1, t2;

	//TODO look at scheduling to see if we can run and merge in background
	//we should not merge to a running video
	//TODO refactor into seperate method
	this->cf.clearScreen();

	t1 = high_resolution_clock::now();
	file = copyEllipses(CAP_FILE, this->mix_file, this->ellipses_in, this->ellipses_out, this->fileonly);
	t2 = high_resolution_clock::now();

	auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	std::cout << "merging file took: " << int_ms.count() << " milliseconds" << endl;

	if (this->filename2.compare("") != 0)
	{
		t1 = high_resolution_clock::now();
		file2 = copyEllipses(CAP_FILE2, this->mix_file2, this->ellipses_in, this->ellipses_out, false);
		t2 = high_resolution_clock::now();

		auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "merging file2 took: " << int_ms.count() << " milliseconds" << endl;

		t1 = high_resolution_clock::now();
		morph = this->morph(ImgToMat(&file[file.size()-1]),ImgToMat(&file2[0]),this->morphsteps);

		t2 = high_resolution_clock::now();

		int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "morphing file and file2 took: " << int_ms.count() << " milliseconds" << endl;

		while (file.size() % VIDEO_FPS != 0) file.push_back(file[(file.size()-1)]);
		for (unsigned int i = 0; i < filesteps; i++)
			std::copy(file.begin(), file.end(), std::back_inserter(totaal));
		while (morph.size() % VIDEO_FPS != 0) morph.push_back(morph[(morph.size()-1)]);
			std::copy(morph.begin(), morph.end(), std::back_inserter(totaal));
		while (file2.size() % VIDEO_FPS != 0) file2.push_back(file2[(file2.size()-1)]);
		for (unsigned int i = 0; i < file2steps; i++)
			std::copy(file2.begin(), file2.end(), std::back_inserter(totaal));
	}
	else
	{
		for (unsigned int i = 0; i < filesteps; i++)
			while (file.size() % VIDEO_FPS != 0) file.push_back(file[(file.size()-1)]);


		/*
		for (unsigned int i = 0; i < 10; i++)
		{
			file.push_back(file[(file.size()-1)]);
		}*/

		for (unsigned int i = 0; i < file.size(); i++)
		{
			std::copy(file.begin(), file.end(), std::back_inserter(totaal));
		}
	}

	fprintf(stderr,"starting encoding...\n");
    std::string recordname = TMP_DIR + this->getUuid() + "." + VIDEO_EXT;

	//av_log_set_level(AV_LOG_DEBUG);

	t1 = high_resolution_clock::now();	t1 = high_resolution_clock::now();

	/*
    VideoWriter record(recordname, CV_FOURCC('M','P','4','V'),
			  VIDEO_FPS, cv::Size(1024,768), true);


    for (unsigned int i = 0; i < totaal.size(); i++) {
    	record.write(ImgToMat(&totaal[i]));
    }
	*/

	/*
	fprintf(stderr,"av_register_all call\n");

    av_register_all();

	fprintf(stderr,"avcodec_find_encoder_by_name\n");

    // auto codec = avcodec_find_encoder_by_name( "libx264" ); // works
    auto codec = avcodec_find_encoder_by_name( "h264_omx" );
    if( !codec )
    {
        throw std::runtime_error( "Unable to find codec" );
    }

	fprintf(stderr,"avcodec_alloc_context3\n");

    auto context =
        std::shared_ptr< AVCodecContext >( avcodec_alloc_context3( codec ), freeContext );

    if( !context )
    {
        throw std::runtime_error( "Unable to allocate context" );
    }

    std::cout << "Setting options" << std::endl;

    //context->bit_rate = 400 * 1024;  // 400 KBit/s
    context->width = 640;
    context->height = 480;
    //context->pix_fmt = AV_PIX_FMT_YUV420P;
    context->time_base.num = 30; // milliseconds
    context->time_base.den = 1;
    //context->thread_count = 0;
    //context->level = 31;

    // av_opt_set( m_context->priv_data, "tune", "zerolatency", 0 );
    av_opt_set( context->priv_data, "preset", "slow", 0 );

    std::cout << "Opening context" << std::endl;

    auto errorCode = avcodec_open2( context.get(), codec, nullptr );
    if( errorCode < 0 )
    {
        throw std::runtime_error( "Unable to open codec (" + avError( errorCode ) + ")" );
    }

    std::cout << "Done" << std::endl;

    */

    // initialize FFmpeg library
    av_register_all();
	//av_log_set_level(AV_LOG_DEBUG);
    int ret;

    const int dst_width = VIDEO_WIDTH;
    const int dst_height = VIDEO_HEIGHT;
    const AVRational dst_fps = { VIDEO_FPS,	1 };
    const AVRational dst_timebase = { 1, VIDEO_FPS };

    // allocate cv::Mat with extra bytes (required by AVFrame::data)
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16);
    cv::Mat image(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);

    // open output format context
    AVFormatContext* outctx = nullptr;
    ret = avformat_alloc_output_context2(&outctx, nullptr, nullptr, recordname.c_str());
    if (ret < 0) {
        std::cerr << "fail to avformat_alloc_output_context2(" << recordname.c_str() << "): ret=" << ret;
        return;
    }

    // open output IO context
    ret = avio_open2(&outctx->pb, recordname.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avio_open2: ret=" << ret;
        return;
    }

    // create new video stream
    //AVCodec* vcodec = avcodec_find_encoder(outctx->oformat->video_codec);
    AVCodec* vcodec = avcodec_find_encoder_by_name("h264_omx");
    if (!vcodec)
    {
    	fprintf(stderr,"Kan codec nie vinden nie \n");
    	return;
    }

    AVStream* vstrm = avformat_new_stream(outctx, vcodec);
    if (!vstrm) {
        std::cerr << "fail to avformat_new_stream";
        return;
    }

    avcodec_get_context_defaults3(vstrm->codec, vcodec);
    vstrm->codec->width = dst_width;
    vstrm->codec->height = dst_height;
    //vstrm->codec->level = 32;
    //vstrm->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    vstrm->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    //vstrm->codec->pix_fmt = vcodec->pix_fmts[0];
    //vstrm->codec->time_base = vstrm->time_base = av_inv_q(dst_fps);
    vstrm->codec->time_base = dst_timebase;
    vstrm->codec->framerate = dst_fps;
    //vstrm->codec->max_b_frames = 1;
    vstrm->codec->gop_size = 10;
    //vstrm->codec->compression_level = 0;
    //vstrm->r_frame_rate = vstrm->avg_frame_rate = dst_fps;
    //vstrm->codec->thread_count = 0;
    vstrm->codec->bit_rate = 4000 * 1024;
    //vstrm->codec->delay = 0;
    //vstrm->codec->max_b_frames = 0;
    //vstrm->codec->thread_count = 1;

   	//av_opt_set(vstrm->codec->priv_data, "preset", "slow", 0);
   	//av_opt_set(vstrm->codec->priv_data, "tune", "film", 0);
    //av_opt_set(vstrm->codec->priv_data, "crf", "1", AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(vstrm->codec->priv_data, "qp", "10", 0);

    //vstrm->codec->flags &= ~AV_CODEC_CAP_DELAY;
    //fprintf(stderr,"Capabilities %i",vstrm->codec->codec->capabilities);
    if (outctx->oformat->flags & AVFMT_GLOBALHEADER)
        vstrm->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open video encoder
    ret = avcodec_open2(vstrm->codec, vcodec, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avcodec_open2: ret=" << ret;
        return;
    }

    std::cout
        << "outfile: " << recordname.c_str() << "\n"
        << "format:  " << outctx->oformat->name << "\n"
        << "vcodec:  " << vcodec->name << "\n"
        << "size:    " << dst_width << 'x' << dst_height << "\n"
        << "fps:     " << av_q2d(dst_fps) << "\n"
        << "pixfmt:  " << av_get_pix_fmt_name(vstrm->codec->pix_fmt) << "\n"
        << std::flush;

    // initialize sample scaler
    SwsContext* swsctx = sws_getCachedContext(
        nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
        dst_width, dst_height, vstrm->codec->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        return;
    }

    // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> framebuf(avpicture_get_size(vstrm->codec->pix_fmt, dst_width, dst_height));
    avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), vstrm->codec->pix_fmt, dst_width, dst_height);
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(vstrm->codec->pix_fmt);

    // encoding loop
    avformat_write_header(outctx, nullptr);
    int64_t frame_pts = 0;
    unsigned nb_frames = 0;
    int got_pkt = 0;

    for (unsigned int i = 0; i < totaal.size(); i++) {
   		ImgToMat(&totaal[i]).copyTo(image);
        const int stride[] = { static_cast<int>(image.step[0]) };
        sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
        frame->pts = frame_pts++;
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, frame, &got_pkt);
        //fprintf(stderr, "image size is %i\n",pkt.size);
        if (ret < 0) {
            std::cerr << "fail to avcodec_encode_video2: ret=" << ret << "\n";
            break;
        }
        if (got_pkt) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
            av_free_packet(&pkt);
        }
    }

    do {
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, NULL, &got_pkt);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_pkt) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
            av_free_packet(&pkt);
        }
    } while (got_pkt);

    av_write_trailer(outctx);
    std::cout << nb_frames << " frames encoded" << std::endl;

    av_frame_free(&frame);
    avcodec_close(vstrm->codec);
    avio_close(outctx->pb);
    avformat_free_context(outctx);

    t2 = high_resolution_clock::now();
	int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	std::cout << "H264 encoding took: " << int_ms.count() << " milliseconds" << endl;
}


std::string CloneFactory::Clone::getUuid(){
	char uuid_str[37];
	uuid_unparse(uuid,uuid_str);
	return uuid_str;
}

std::string CloneFactory::Clone::getUrl(){
	return url;
}

CloneFactory::Clone* CloneFactory::addClone(std::string naam, std::string omschrijving){
	CloneFactory::Clone * clone = new CloneFactory::Clone(*this, naam, omschrijving);
	std::string uuid_str = clone->getUuid();
	clonemap.insert(std::make_pair(uuid_str,clone));
	return clone;
}

void CloneFactory::deleteClone(std::string uuid){
	std::map<std::string, CloneFactory::Clone*>::iterator it = clonemap.begin();
    it = clonemap.find(uuid);
	if (it != clonemap.end())
	{
	    // found it - delete it
	    delete it->second;
	    clonemap.erase(it);
	}
}

void CloneFactory::Clone::Stop(){
	delay(500);
}

void CloneFactory::Clone::Start(){
	delay(500);
}

std::string CloneFactory::Clone::getNaam(){
	return naam;
}

std::string CloneFactory::Clone::getOmschrijving(){
	return omschrijving;
}

bool CloneFactory::CloneFactoryHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CloneFactory::CloneFactoryHandler::handleAll("GET", server, conn);
	}

bool CloneFactory::CloneFactoryHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CloneFactory::CloneFactoryHandler::handleAll("POST", server, conn);
	}

bool CloneFactory::Clone::CloneHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return CloneFactory::Clone::CloneHandler::handleAll("GET", server, conn);
	}

bool CloneFactory::Clone::CloneHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return CloneFactory::Clone::CloneHandler::handleAll("POST", server, conn);
	}

bool CloneFactory::CloneFactoryHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string dummy;
	std::string value;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream tohead;

	if(CivetServer::getParam(conn, "delete", value))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"0;url=/clonefactory\" />";
	   this->clonefactory.deleteClone(value);
	}
	else
	if(CivetServer::getParam(conn, "save", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/clonefactory\" />";
	   message = _("Saved!");
       this->clonefactory.save();
	}
	else
	if(CivetServer::getParam(conn, "load", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/clonefactory\" />";
	   message = _("Loaded!");
	   this->clonefactory.load();
	}
	else
	/* if parameter clear is present the clear screen button was pushed */
	if(CivetServer::getParam(conn, "clear", dummy))
	{
	   meta = "<meta http-equiv=\"refresh\" content=\"1;url=/clonefactory\" />";
	   message = _("Screen empty!");
       this->clonefactory.clearScreen();
	}
	else if(CivetServer::getParam(conn, "newselect", dummy))
	{
	  CivetServer::getParam(conn, "naam", value);
	  std::string naam = value;
	  CivetServer::getParam(conn, "omschrijving", value);
	  std::string omschrijving = value;

      CloneFactory::Clone* clone = clonefactory.addClone(naam, omschrijving);

      meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + clone->getUrl() + "\" />";
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "new", dummy))
	{
	   ss << "<form action=\"/clonefactory\" method=\"POST\">";
	   ss << "<div class=\"container\">";
	   ss << "<label for=\"naam\">" << _("Name") << "</label>"
  			 "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" name=\"naam\"/>" << "</br>";
	   ss << "<label for=\"omschrijving\">" << _("Comment") << "</label>"
	         "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" name=\"omschrijving\"/>" << "</br>";
	   ss << "</div>";
	   ss << "<button type=\"submit\" name=\"newselect\" value=\"newselect\" ";
   	   ss << "id=\"newselect\">" << _("Add") << "</button>&nbsp;";
   	   ss << "</form>";

	}
	/* initial page display */
	else
	{
		std::map<std::string, CloneFactory::Clone*>::iterator it = clonefactory.clonemap.begin();
		for (std::pair<std::string, CloneFactory::Clone*> element : clonefactory.clonemap) {
			ss << "<br style=\"clear:both\">";
			ss << "<div class=\"row\">";
			ss << _("Name") << ":&nbsp;" << element.second->getNaam() << " &nbsp;";
			ss << _("Comment") << ":&nbsp;" << element.second->getOmschrijving() << " &nbsp;";
			ss << "<br style=\"clear:both\">";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"" << element.second->getUrl() << "\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"select\" id=\"select\">" << _("Select") << "</button>&nbsp;";
	    	ss << "</form>";
	    	ss << "<form style ='float: left; margin: 0px; padding: 0px;' action=\"/clonefactory\" method=\"POST\">";
	    	ss << "<button type=\"submit\" name=\"delete\" value=\"" << element.first << "\" id=\"delete\">" << _("Remove") << "</button>&nbsp;";
			ss << "</form>";
			ss << "<br style=\"clear:both\">";
			ss << "</div>";
	    }
	    ss << "<br>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/clonefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">" << _("New") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/clonefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"save\" id=\"save\">" << _("Save") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/clonefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"load\" id=\"load\">" << _("Load") << "</button>";
	    ss << "</form>";
	    ss << "<form style ='float: left; padding: 0px;' action=\"/clonefactory\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"clear\" id=\"clear\">" << _("Clear Screen") << "</button>";
	    ss << "</form>";
	    ss << "<br style=\"clear:both\">";
	    ss << "<br>";
	}

	ss = getHtml(meta, message, "clone",  ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");

	return true;
}

bool CloneFactory::Clone::CloneHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string value;
	std::string message="&nbsp;";
	std::string meta="";
	std::stringstream tohead;

	/* if parameter submit_action is present we want to add an action */
	if(CivetServer::getParam(conn, "add_action", dummy))
	{
		   CivetServer::getParam(conn,"centerx", s[0]);
		   CivetServer::getParam(conn,"centery", s[1]);
		   CivetServer::getParam(conn,"axeheight", s[2]);
		   CivetServer::getParam(conn,"axewidth", s[3]);
		   CivetServer::getParam(conn,"centerxo", s[4]);
		   CivetServer::getParam(conn,"centeryo", s[5]);
		   CivetServer::getParam(conn,"axeheighto", s[6]);
		   CivetServer::getParam(conn,"axewidtho", s[7]);

		   ellipse_s elli, ello;

           elli.centerx = atoi(s[0].c_str());
           elli.centery = atoi(s[1].c_str());
           elli.axeheight = atof(s[2].c_str());
           elli.axewidth = atof(s[3].c_str());
		   clone.ellipses_in->push_back(elli);

           ello.centerx = atoi(s[4].c_str());
           ello.centery = atoi(s[5].c_str());
           ello.axeheight = atof(s[6].c_str());
           ello.axewidth = atof(s[7].c_str());
		   clone.ellipses_out->push_back(ello);

		   meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + clone.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "update_action", value))
	{
		   CivetServer::getParam(conn,"centerx", s[0]);
		   CivetServer::getParam(conn,"centery", s[1]);
		   CivetServer::getParam(conn,"axeheight", s[2]);
		   CivetServer::getParam(conn,"axewidth", s[3]);
		   CivetServer::getParam(conn,"centerxo", s[4]);
		   CivetServer::getParam(conn,"centeryo", s[5]);
		   CivetServer::getParam(conn,"axeheighto", s[6]);
		   CivetServer::getParam(conn,"axewidtho", s[7]);

           (*clone.ellipses_in)[atoi(value.c_str())].centerx = atoi(s[0].c_str());
           (*clone.ellipses_in)[atoi(value.c_str())].centery = atoi(s[1].c_str());
           (*clone.ellipses_in)[atoi(value.c_str())].axeheight = atof(s[2].c_str());
           (*clone.ellipses_in)[atoi(value.c_str())].axewidth = atof(s[3].c_str());

           (*clone.ellipses_out)[atoi(value.c_str())].centerx = atoi(s[4].c_str());
           (*clone.ellipses_out)[atoi(value.c_str())].centery = atoi(s[5].c_str());
           (*clone.ellipses_out)[atoi(value.c_str())].axeheight = atof(s[6].c_str());
           (*clone.ellipses_out)[atoi(value.c_str())].axewidth = atof(s[7].c_str());

		   meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + clone.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "delete", value))
	{
		clone.ellipses_in->erase(clone.ellipses_in->begin() + atoi(value.c_str()));
		clone.ellipses_out->erase(clone.ellipses_out->begin() + atoi(value.c_str()));

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=\"" + clone.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "naam", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.naam = value.c_str();
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
		clone.omschrijving = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "mix_file", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.mix_file = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "mix_file2", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.mix_file2 = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "filesteps", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.filesteps = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "file2steps", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.file2steps = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "morphsteps", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.morphsteps = atoi(value.c_str());
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "filename", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.filename = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "filename2", value))
	{
		CivetServer::getParam(conn,"value", value);
		clone.filename2 = value.c_str();
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "clonetofile", value))
	{
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			clone.clone_to_file = true;
		else
			clone.clone_to_file = false;
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "clonetofile2", value))
	{
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			clone.clone_to_file2 = true;
		else
			clone.clone_to_file2 = false;
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "fileonly", value))
	{
		CivetServer::getParam(conn,"value", value);
		if (value.compare("true") == 0)
			clone.fileonly = true;
		else
			clone.fileonly = false;
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << value;
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "ellipseonframe", value))
	{
		if (value.compare("true") == 0)
			{
				CivetServer::getParam(conn,"frames", value);
				clone.getFrame(atoi(value.c_str()),true);
			}
		else
			{
				CivetServer::getParam(conn,"frames", value);
				clone.getFrame(atoi(value.c_str()), false);
			}
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << "tmp/" << clone.getUuid() << "_file.jpg?t=" << std::time(0);
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}
	if(CivetServer::getParam(conn, "frames", value))
	{
		std::string ellipseonframe;
		CivetServer::getParam(conn,"ellipseonframe", ellipseonframe);
		if (ellipseonframe.compare("true") == 0)
					clone.getFrame(atoi(value.c_str()),true);
			else
					clone.getFrame(atoi(value.c_str()), false);
		clone.getFrame(atoi(value.c_str()),true);
		std::stringstream ss;
		ss << "HTTP/1.1 200 OK\r\nContent-Type: ";
		ss << "text/html\r\nConnection: close\r\n\r\n";
		ss << "tmp/" << clone.getUuid() << "_file.jpg?t=" << std::time(0);
		mg_printf(conn, ss.str().c_str(), "%s");
		return true;
	}

	if(CivetServer::getParam(conn, "on_screen", dummy))
	{
		clone.onScreen();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + clone.getUrl() + "\"/>";
		message = _("On Screen!");
	}
    if(CivetServer::getParam(conn, "merge", dummy))
	{
		std::unique_lock<std::mutex> l(m);
		clone.mergeFrames();
		l.unlock();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + clone.getUrl() + "\"/>";
		message = _("Merged!");
	}
	if(CivetServer::getParam(conn, "newfile", value))
	{
		clone.filename = value;
		std::stringstream ss;

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + clone.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "newfile2", value))
	{
		clone.filename2 = value;
		std::stringstream ss;

		meta = "<meta http-equiv=\"refresh\" content=\"0;url=" + clone.getUrl() + "\"/>";
	}
	if(CivetServer::getParam(conn, "save_file", dummy))
	{
		imwrite(FILES_DIR "clone.jpg",(*clone.cf.camMat));

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + clone.getUrl() + "\"/>";
		message = _("File saved!");
	}
	if(CivetServer::getParam(conn, "clone", dummy))
	{
		clone.openCap(CAP_CAM);
		clone.cf.camMat->release();
		(*clone.cf.camMat) = clone.cloneFrame(CAP_CAM);
		clone.closeCap();

		meta = "<meta http-equiv=\"refresh\" content=\"1;url=\"" + clone.getUrl() + "\"/>";
		message = _("Frame Cloned!");
	}

	std::stringstream ss;

	if(CivetServer::getParam(conn, "filesel", dummy))
	{
	   DIR *dirp;
	   struct dirent *dp;
	   ss << "<form action=\"" << clone.getUrl() << "\" method=\"POST\">";
	   if ((dirp = opendir(FILES_DIR)) == NULL) {
	          (*syslog) << "couldn't open " << FILES_DIR << endl;
	   }
	   else
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newfile\" value=\"" << FILES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newfile\">" << _("File") << "</button>&nbsp;";
	    	ss << "<button type=\"submit\" name=\"newfile2\" value=\"" << FILES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newfile2\">" << _("File") << " 2</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "<br>";
	        }
	   } while (dp != NULL);
       ss << "<button type=\"submit\" name=\"annuleren\" value=\"annuleren\" id=\"annuleren\">" << _("Cancel") << "</button>&nbsp;";
       ss << "</form>";
       (void) closedir(dirp);
	}
	else
	if(CivetServer::getParam(conn, "add", value))
	{
	  	ss << "<form action=\"" << clone.getUrl() << "\" method=\"POST\">";
	    ss << "<h2>" << _("Ellipse in") << ":</h2>";
		ss << "<label for=\"centerx\">" << _("centerx") << ":</label>"
					  "<input class=\"inside\" id=\"centerx\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centerx\"/>" << "<br>";
		ss << "<label for=\"centery\">" << _("centery") << ":</label>"
					  "<input class=\"inside\" id=\"centery\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centery\"/>" << "<br>";
		ss << "<label for=\"axeheight\">" << _("axeheight") << ":</label>"
					  "<input class=\"inside\" id=\"axeheight\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axeheight\"/>" << "<br>";
		ss << "<label for=\"axewidth\">" << _("axewidth") << ":</label>"
					  "<input class=\"inside\" id=\"axewidth\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axewidth\"/>" << "<br>";
		ss << "<h2>" << _("Ellipse Out") << ":</h2>";
		ss << "<label for=\"centerxo\">" << _("centerx") << ":</label>"
					  "<input class=\"inside\" id=\"centerxo\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centerxo\"/>" << "<br>";
		ss << "<label for=\"centeryo\">" << _("centery") << ":</label>"
					  "<input class=\"inside\" id=\"centeryo\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centeryo\"/>" << "<br>";
		ss << "<label for=\"axeheighto\">" << _("axeheight") << ":</label>"
					  "<input class=\"inside\" id=\"axeheighto\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axeheighto\"/>" << "<br>";
		ss << "<label for=\"axewidtho\">" << _("axewidth") << ":</label>"
					  "<input class=\"inside\" id=\"axewidtho\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axewidtho\"/>" << "<br>";
		ss << "<button type=\"submit\" name=\"add_action\" value=\"add_action\" id=\"submit\">" << _("Add") << "</button></br>";
		ss << "</br>";
		ss << "</form>";
	}
	else
	if(CivetServer::getParam(conn, "update", value))
	{
	  	ss << "<form action=\"" << clone.getUrl() << "\" method=\"POST\">";
	    ss << "<h2>" << _("Ellipse in") << ":</h2>";
		ss << "<label for=\"centerx\">" << _("centerx") << ":</label>"
					  "<input class=\"inside\" id=\"centerx\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centerx\" value=\"" << (*clone.ellipses_in)[(atoi(value.c_str()))].centerx << "\" />" << "<br>";
		ss << "<label for=\"centery\">" << _("centery") << ":</label>"
					  "<input class=\"inside\" id=\"centery\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centery\" value=\"" << (*clone.ellipses_in)[(atoi(value.c_str()))].centery << "\" />" << "<br>";
		ss << "<label for=\"axeheight\">" << _("axeheight") << ":</label>"
					  "<input class=\"inside\" id=\"axeheight\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axeheight\" value=\"" << (*clone.ellipses_in)[(atoi(value.c_str()))].axeheight << "\" />" << "<br>";
		ss << "<label for=\"axewidth\">" << _("axewidth") << ":</label>"
					  "<input class=\"inside\" id=\"axewidth\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axewidth\" value=\"" << (*clone.ellipses_in)[(atoi(value.c_str()))].axewidth << "\" />" << "<br>";
		ss << "<h2>" << _("Ellipse Out") << ":</h2>";
		ss << "<label for=\"centerxo\">" << _("centerx") << ":</label>"
					  "<input class=\"inside\" id=\"centerxo\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centerxo\" value=\"" << (*clone.ellipses_out)[(atoi(value.c_str()))].centerx << "\" />" << "<br>";
		ss << "<label for=\"centeryo\">" << _("centery") << ":</label>"
					  "<input class=\"inside\" id=\"centeryo\" type=\"number\" min=\"0\" max=\"10000\" placeholder=\"0\" step=\"1\"" <<
					  " name=\"centeryo\" value=\"" << (*clone.ellipses_out)[(atoi(value.c_str()))].centery << "\" />" << "<br>";
		ss << "<label for=\"axeheighto\">" << _("axeheight") << ":</label>"
					  "<input class=\"inside\" id=\"axeheighto\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axeheighto\" value=\"" << (*clone.ellipses_out)[(atoi(value.c_str()))].axeheight << "\" />" << "<br>";
		ss << "<label for=\"axewidtho\">" << _("axewidth") << ":</label>"
					  "<input class=\"inside\" id=\"axewidtho\" type=\"number\" min=\"0\" max=\"1000\" placeholder=\"0.00\" step=\"any\" " <<
					  " name=\"axewidtho\" value=\"" << (*clone.ellipses_out)[(atoi(value.c_str()))].axewidth << "\" />" << "<br>";
		ss << "<button type=\"submit\" name=\"update_action\" value=\"" << value << "\" id=\"submit\">" << _("Update") << "</button></br>";
		ss << "</br>";
		ss << "</form>";
	}
	else
	/* initial page display */
	{
		//tohead << "<script type=\"text/javascript\">";
		//tohead << " $(document).ready(function(){";
		//tohead << "  setInterval(function(){";
		//tohead << "  $.get( \"" << clone.getUrl() << "?streaming=true\", function( data ) {";
		//tohead << "  $( \"#clone\" ).html( data );";
		//tohead << " });},1000)";
		//tohead << "});";
		//tohead << "</script>";
		tohead << "<script type=\"text/javascript\">";
		tohead << " $(document).ready(function(){";
		tohead << " $('#naam').on('change', function() {";
		tohead << " $.get( \"" << clone.getUrl() << "\", { naam: 'true', value: $('#naam').val() }, function( data ) {";
		tohead << "  $( \"#naam\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#omschrijving').on('change', function() {";
		tohead << " $.get( \"" << clone.getUrl() << "\", { omschrijving: 'true', value: $('#omschrijving').val() }, function( data ) {";
		tohead << "  $( \"#omschrijving\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#filename').on('change', function() {";
		tohead << " $.get( \"" << clone.getUrl() << "\", { filename: 'true', value: $('#filename').val() }, function( data ) {";
		tohead << "  $( \"#filename\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#filename2').on('change', function() {";
		tohead << " $.get( \"" << clone.getUrl() << "\", { filename2: 'true', value: $('#filename2').val() }, function( data ) {";
		tohead << "  $( \"#filename2\" ).html( data );})";
	    tohead << "});";
		tohead << " $('#mix_file').on('change', function() {";
		tohead << " $.get( \"" << clone.getUrl() << "\", { mix_file: 'true', value: $('#mix_file').val() }, function( data ) {";
		tohead << "  $( \"#mix_file\" ).html( data );})";
	    tohead << "});";
	    tohead << " $('#mix_file2').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { mix_file2: 'true', value: $('#mix_file2').val() }, function( data ) {";
   		tohead << "  $( \"#mix_file2\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#filesteps').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { filesteps: 'true', value: $('#filesteps').val() }, function( data ) {";
   		tohead << "  $( \"#filesteps\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#file2steps').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { file2steps: 'true', value: $('#file2steps').val() }, function( data ) {";
   		tohead << "  $( \"#file2steps\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#morphsteps').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { morphsteps: 'true', value: $('#morphsteps').val() }, function( data ) {";
   		tohead << "  $( \"#morphsteps\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#clonetofile').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { clonetofile: 'true', value: $('#clonetofile').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#clonetofile\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#clonetofile2').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { clonetofile2: 'true', value: $('#clonetofile2').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#clonetofile2\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#fileonly').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { fileonly: 'true', value: $('#fileonly').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#fileonly\" ).html( data );})";
  	    tohead << "});";
	    tohead << " $('#frames').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { frames: $('#frames').val(), ellipseonframe: $('#ellipseonframe').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#photo\" ).attr( 'src' , data );})";
  	    tohead << "});";
  	    tohead << " $('#ellipseonframe').on('change', function() {";
   		tohead << " $.get( \"" << clone.getUrl() << "\", { frames: $('#frames').val(), ellipseonframe: $('#ellipseonframe').is(':checked') }, function( data ) {";
   		tohead << "  $( \"#photo\" ).attr( 'src' , data );})";
   		tohead << "});";
		tohead << "});";
		tohead << "</script>";
		ss << "<form action=\"" << clone.getUrl() << "\" method=\"POST\">";
	    ss << "<button type=\"submit\" name=\"refresh\" value=\"refresh\" id=\"refresh\">" << _("Refresh") << "</button><br>";
	    ss <<  "<br>";
	    ss << "<button type=\"submit\" name=\"clone\" value=\"clone\" id=\"clone_button\">" << _("Input Cam") << "</button>";
	    ss << "<button type=\"submit\" name=\"save_file\" value=\"save_file\" id=\"save_file\">" << _("Save as File") << "</button>";
	    ss << "<button type=\"submit\" name=\"filesel\" id=\"filesel\">" << _("Input File") << "</button>";
	    ss << "<button type=\"submit\" name=\"merge\" id=\"merge\">" << _("Merge") << "</button>";
	    ss << "<button type=\"submit\" name=\"on_screen\" id=\"on_screen\">" << _("On Screen") << "</button>";
	    ss << "</form>";
		ss << "<div class=\"container\">";
		ss << "<label for=\"naam\">" << _("Name") << ":</label>"
					  "<input class=\"inside\" id=\"naam\" type=\"text\" size=\"20\" value=\"" <<
					  clone.naam << "\" name=\"naam\"/>" << "<br>";
		ss << "<label for=\"omschrijving\">" << _("Comment") << ":</label>"
					  "<input class=\"inside\" id=\"omschrijving\" type=\"text\" size=\"30\" value=\"" <<
					  clone.omschrijving << "\" name=\"omschrijving\"/>" << "<br>";
		ss << "<label for=\"filename\">" << _("Filename") << ":</label>"
					  "<input class=\"inside\" id=\"filename\" type=\"text\" size=\"50\" value=\"" <<
					  clone.filename << "\" name=\"filename\"/>" << "</br>";
		if (clone.clone_to_file)
		{
			ss << "<label for=\"clonetofile\">" << _("Clone to file") << ":</label>"
			   	   	  "<input id=\"clonetofile\" type=\"checkbox\" name=\"clonetofile\" value=\"ja\" checked/>" << "</br></br>";
		}
		else
		{
			ss << "<label for=\"clonetofile\">" << _("Clone to file") << ":</label>"
			   	   	  "<input id=\"clonetofile\" type=\"checkbox\" name=\"clonetofile\" value=\"ja\"/>" << "</br></br>";
		}
		ss << "<label for=\"filename2\">" << _("Filename") << "2:</label>"
					  "<input class=\"inside\" id=\"filename2\" type=\"text\" size=\"50\" value=\"" <<
					  clone.filename2 << "\" name=\"filename2\"/>" << "</br>";
		if (clone.clone_to_file2)
		{
			ss << "<label for=\"clonetofile2\">" << _("Clone to file") << "2:</label>"
			   	   	  "<input id=\"clonetofile2\" type=\"checkbox\" name=\"clonetofile2\" value=\"ja\" checked/>" << "</br></br>";
		}
		else
		{
			ss << "<label for=\"clonetofile2\">" << _("Clone to file") << "2:</label>"
			   	   	  "<input id=\"clonetofile2\" type=\"checkbox\" name=\"clonetofile2\" value=\"ja\"/>" << "</br></br>";
		}

		if (clone.fileonly)
		{
			ss << "<label for=\"fileonly\">" << _("File Only") << ":</label>"
			   	   	  "<input id=\"fileonly\" type=\"checkbox\" name=\"fileonly\" value=\"ja\" checked/>" << "</br>";
		}
		else
		{
			ss << "<label for=\"fileonly\">" << _("File Only") << ":</label>"
			   	   	  "<input id=\"fileonly\" type=\"checkbox\" name=\"fileonly\" value=\"ja\"/>" << "</br>";
		}
	    ss << "<br>";
		ss << "<label for=\"mix_file\">" << _("Mix file") << ":</label>";
		ss << "<td><input class=\"inside\" id=\"mix_file\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			   clone.mix_file << "\"" << " name=\"mix_file\" /><br>";
		ss << "<label for=\"mix_to\">" << _("Mix file") << "2:</label>";
		ss << "<td><input class=\"inside\" id=\"mix_file2\" type=\"range\" min=\"1\" max=\"100\" step=\"1\" value=\"" <<
			  clone.mix_file2 << "\"" << " name=\"mix_file2\" /><br>";
		ss << "</div>";
		ss << "<label for=\"filesteps\">" << _("Filesteps") << ":</label>";
		ss << "<td><input class=\"inside\" id=\"filesteps\" type=\"number\" min=\"1\" max=\"100\" placeholder=\"1\" step=\"1\" value=\"" <<
			  clone.filesteps << "\"" << " name=\"filesteps\" /><br>";
		ss << "</div>";
		ss << "<label for=\"file2steps\">" << _("Filesteps") << "2:</label>";
		ss << "<td><input class=\"inside\" id=\"file2steps\" type=\"number\" min=\"1\" max=\"100\" placeholder=\"1\" step=\"1\" value=\"" <<
			  clone.file2steps << "\"" << " name=\"file2steps\" /><br>";
		ss << "</div>";
		ss << "<label for=\"morphsteps\">" << _("Morphsteps") << ":</label>";
		ss << "<td><input class=\"inside\" id=\"morphsteps\" type=\"number\" min=\"1\" max=\"100\" placeholder=\"1\" step=\"1\" value=\"" <<
			  clone.morphsteps << "\"" << " name=\"morphsteps\" /><br>";
		ss << "</div>";
		ss << "<br>";
	    ss << "</br>";
		ss << "<form action=\"" << clone.getUrl() << "\" method=\"POST\">";
	    ss << "<table class=\"rechts\">";
	    ss << "<thead><tr><th class=\"kort\"><div class=\"waarde\"><button type=\"submit\" name=\"add\" value=\"-1\" id=\"add\">&#8627;</button></div></th>";
	    ss << "<th class=\"kort\"><div class=\"waarde\">&nbsp;</div></th><th class=\"kort\"><div class=\"waarde\">&nbsp;</div></th><th class=\"kort\"><div class=\"waarde\">" << _("Ellipse Number") << "</div></th><th><div class=\"waarde\">" << _("Ellipse In") << "</div></th><th><div class=\"waarde\">" << _("Ellipse Out") << "</div></th></tr></thead>";
		for (unsigned int i = 0; i < clone.ellipses_in->size(); i++)
		{
			ss << "<tr>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"add\" value=\"" << i << "\" id=\"add\">&#8627;</button></div></td>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"update\" value=\"" << i << "\" id=\"update\">&#x270e;</button></div></td>";
			ss << "<td class=\"kort\"><div class=\"waarde\">" << "<button type=\"submit\" name=\"delete\" value=\"" << i << "\" id=\"delete\" style=\"font-weight:bold\">&#x2718</button></div></td>";
			ss << "<td class=\"kort\"><div class=\"waarde\" id=\"number\">" << i+1 << "</td>";
			ss << "<td class=\"kort\"><div class=\"waarde\" id=\"number\">" << (*clone.ellipses_in)[i].centerx << " x " << (*clone.ellipses_in)[i].centery << "</div></td>";
			ss << "<td class=\"kort\"><div class=\"waarde\" id=\"number\">" << (*clone.ellipses_out)[i].centerx << " x " << (*clone.ellipses_out)[i].centery << "</div></td>";
			ss << "<tr>";
		}
		ss << "</table>";
	    ss << "</form>";

	    /*
	    ss << "<div id=\"clone\">";
	    ss << "<img src=\"" << clone.url << "/?streaming=true\">";
	    ss << "</div>"; */
	    ss << "<h2>" << _("Photo") << ":</h2>";
	    ss << "<img src=\"" << "tmp/" << clone.getUuid() << "_photo.jpg?t=" << std::time(0) << "\"></img><br>";
		if (!clone.filename.empty())
	    {
	    	clone.openCap(CAP_FILE);
	    	int number_of_frames = clone.cap->get(CAP_PROP_FRAME_COUNT);
	    	clone.closeCap();
	    	//l.unlock();
			//int number_of_frames = clone.filePoints->size();
	    	if (number_of_frames)
	    	{
	    		ss << "<h2>" << _("File") << ":</h2>";
	    		ss << "<form oninput=\"result.value=parseInt(frames.value)\">";
	    		ss << "<label for=\"frames\">" << _("Frame") << ":</label>";
	    		ss << "<output name=\"result\">1</output><br>";
	    		ss << "<input class=\"inside\" id=\"frames\" type=\"range\" min=\"1\" max=\"" << number_of_frames << "\" step=\"1\" value=\"1\" name=\"frames\" />";
	    		ss << "</form>";
				ss << "<label for=\"ellipseonframe\">" << _("Render faces on frame") << ":</label>"
				   	   	  "<input id=\"ellipseonframe\" type=\"checkbox\" name=\"ellipseonframe\" value=\"ja\"/>" << "</br>";
	    		ss << "<img id=\"photo\" src=\"" << "tmp/" << clone.getUuid() << "_file.jpg?t=" << std::time(0) << "\"></img><br>";
	    		ss << "<br>";
	    	}
	    }
	    ss << "<h2>" << _("Video") << ":</h2>";
	    ss << "<video width=\"" << VIDEO_WIDTH << "\" height=\"" << VIDEO_HEIGHT << "\" controls>";
	    ss << " <source src=\"" << "tmp/" << clone.getUuid() << "." << VIDEO_EXT << "?t=" << std::time(0) << "\" type=\"video/" << VIDEO_EXT << "\">";
	    ss << "Your browser does not support the video tag";
		ss << "</video>";
		ss << "<br>";
	}

	ss = getHtml(meta, message, "clone", ss.str().c_str(), tohead.str().c_str());
    mg_printf(conn, ss.str().c_str(), "%s");
	return true;
}

