/*
 * CaptureFactory.cpp
 *
 *  Created on: April 2, 2017
 *      Author: erik
 *
 *
 */

#include "CaptureFactory.hpp"
using namespace std;
using namespace cv;
using namespace dlib;

// Calculate Delaunay triangles for set of points
// Returns the vector of indices of 3 points for each triangle
static void calculateDelaunayTriangles(Rect rect, std::vector<Point> &points, std::vector< std::vector<int> > &delaunayTri){

	// Create an instance of Subdiv2D
    Subdiv2D subdiv(rect);

	// Insert points into subdiv
    for( std::vector<Point>::iterator it = points.begin(); it != points.end(); it++)
        subdiv.insert(*it);

	std::vector<Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);
	std::vector<Point> pt(3);
	std::vector<int> ind(3);

	for( size_t i = 0; i < triangleList.size(); i++ )
	{
		Vec6f t = triangleList[i];
		pt[0] = Point(t[0], t[1]);
		pt[1] = Point(t[2], t[3]);
		pt[2] = Point(t[4], t[5 ]);

		if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])){
			for(int j = 0; j < 3; j++)
				for(size_t k = 0; k < points.size(); k++)
					if(abs(pt[j].x - points[k].x) < 1.0 && abs(pt[j].y - points[k].y) < 1)
						ind[j] = k;

			delaunayTri.push_back(ind);
		}
	}

}

std::vector <cv::Point> get_points(const dlib::full_object_detection& d)
{
    std::vector <cv::Point> points;
    for (int i = 0; i < 68; ++i)
    {
    	points.push_back(cv::Point(d.part(i).x(), d.part(i).y()));
    }

    return points;
}

void draw_polyline(cv::Mat &img, const dlib::full_object_detection& d, const int start, const int end, bool isClosed = false)
{
    std::vector <cv::Point> points;
    for (int i = start; i <= end; ++i)
    {
        points.push_back(cv::Point(d.part(i).x(), d.part(i).y()));
    }
    cv::polylines(img, points, isClosed, cv::Scalar(255,0,0), 2, 16);
}

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
	this->input = new cv::Mat();
	this->cap = new cv::VideoCapture();

	std::thread( [this] { loadModel(); } ).detach();
	std::thread( [this] { Initialize(); } ).detach();

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
	this->input = new cv::Mat();
	this->cap = new cv::VideoCapture();

	std::thread( [this] { loadModel(); } ).detach();
	std::thread( [this] { Initialize(); } ).detach();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::~Capture(){
	delete mh;
	delete cap;
	delete detector;
	delete pose_model;
	delete input;
	delete output;
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

void CaptureFactory::Capture::loadModel(){
	frontal_face_detector detector = get_frontal_face_detector();
    this->detector = new frontal_face_detector(detector);
    this->pose_model = new shape_predictor();
    cout << "Reading in shape predictor..." << endl;
    deserialize("shape_predictor_68_face_landmarks.dat") >> *pose_model;
    cout << "Done reading in shape predictor ..." << endl;
    model_loaded = true;
}

void CaptureFactory::Capture::Initialize(){
	/*
	 * set relay to output and full stop
	 */
	original.str(std::string());
	original.clear();
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
    *cap >> *input;
    cap->release();


    std::vector<uchar> buf;
    cv::imencode(".png", *input, buf, std::vector<int>() );

    // Base64 encode the stringstream
    base64::encoder E;
    stringstream encoded;
    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    E.encode(incoming, encoded);

    original << encoded.rdbuf();
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */
}

void CaptureFactory::Capture::detect()
{
	std::vector<dlib::rectangle> faces;
    cout << "Detecting faces..." << endl;
    // Detect faces
    cv_image<bgr_pixel> cimg(*input);
    faces = (*detector)(cimg);
    // Find the pose of each face.
    std::vector<full_object_detection> shapes;
    if (faces.size() == 0)
    {
    	cout << "No faces detected." << endl;
        return;
    }
    full_object_detection shape;
    cout << "There were " << faces.size() << " faces detected." << endl;
    for (unsigned long i = 0; i < faces.size(); ++i)
    {
    	// Resize obtained rectangle for full resolution image.
    	     dlib::rectangle r(
    	                   (long)(faces[i].left() * FACE_DOWNSAMPLE_RATIO),
    	                   (long)(faces[i].top() * FACE_DOWNSAMPLE_RATIO),
    	                   (long)(faces[i].right() * FACE_DOWNSAMPLE_RATIO),
    	                   (long)(faces[i].bottom() * FACE_DOWNSAMPLE_RATIO)
    	                );

    	// Landmark detection on full sized image
    	shape = (*pose_model)(cimg, r);
        //shapes.push_back(pose_model(cimg, faces[i]));
        shapes.push_back(shape);

    }

    //Read points
    std::vector<Point> points;
    points = get_points(shape);

    //find convex hull
    std::vector<Point> hull;
    std::vector<int> hullIndex;

    cout << "Finding convex hull." << endl;

    convexHull(points, hullIndex, false, false);

    for(int i = 0; i < (int)hullIndex.size(); i++)
    {
        hull.push_back(points[hullIndex[i]]);
    }

    cv::Mat output = input->clone();
    this->output = new cv::Mat(output);

    //cv::polylines(output, hull, true, cv::Scalar(255,0,0), 2, 16);
    //draw_polyline(output, shape, 0, shape.num_parts(),true);

    draw_polyline(output, shape, 0, 16);           // Jaw line
    draw_polyline(output, shape, 17, 21);          // Left eyebrow
    draw_polyline(output, shape, 22, 26);          // Right eyebrow
    draw_polyline(output, shape, 27, 30);          // Nose bridge
    draw_polyline(output, shape, 30, 35, true);    // Lower nose
    draw_polyline(output, shape, 36, 41, true);    // Left eye
    draw_polyline(output, shape, 42, 47, true);    // Right Eye
    draw_polyline(output, shape, 48, 59, true);    // Outer lip
    draw_polyline(output, shape, 60, 67, true);    // Inner lip

	manipulated.str(std::string());
	manipulated.clear();

    std::vector<uchar> buf;
    cv::imencode(".png", output, buf, std::vector<int>() );

    // Base64 encode the stringstream
    base64::encoder E;
    stringstream encoded;
    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    E.encode(incoming, encoded);

    manipulated << encoded.rdbuf();

    cout << "Finding delaunay triangulation." << endl;

    // Find delaunay triangulation for points on the convex hull
    std::vector< std::vector<int> > dt;
    Rect rect(0, 0, output.cols, output.rows);
    calculateDelaunayTriangles(rect, hull, dt);

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
	mg_printf(conn,
	          "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/html\r\nConnection: close\r\n\r\n");

	if(CivetServer::getParam(conn, "running", dummy))
	{
		std::stringstream ss;
	    if (capture.model_loaded)
	    	ss << "Model Loaded!<br>";
	    else
	    	ss << "Model Loading...<br>";
	    ss << "<img alt=\"Original Image\" src=\"data:image/png;base64," << capture.original.str() << "\"/><br>";
	    ss << "<img alt=\"Manipulated Image\" src=\"data:image/png;base64," << capture.manipulated.str() << "\"/><br>";
		mg_printf(conn, ss.str().c_str());
		return true;
	}

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
	else if(CivetServer::getParam(conn, "capture", dummy))
	{
		capture.Initialize();
		std::stringstream ss;
	//	ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Starten...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "detect", dummy))
	{
		capture.detect();
		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Stoppen...!</h2>");
	}
	else
	{
		std::stringstream ss;
		ss << "<html><head>";
	   	mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "<h2>&nbsp;</h2>");
	}
	/* initial page display */
	{
		std::stringstream ss;
		ss << "<script type=\"text/javascript\" src=\"resources/jquery-3.2.0.min.js\"></script>";
		ss << "<script type=\"text/javascript\">";
		   ss << " $(document).ready(function(){";
		   ss << "  setInterval(function(){";
		   ss << "  $.get( \"" << capture.getUrl() << "?running=true\", function( data ) {";
		   ss << "  $( \"#capture\" ).html( data );";
		   ss << " });},1000)";
		   ss << "});";
		ss << "</script>";
		ss << "</head><body>";
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
	    ss << "<button type=\"submit\" name=\"capture\" value=\"capture\" id=\"capture_button\">CAPTURE</button>";
	    ss << "<button type=\"submit\" name=\"detect\" value=\"detect\" id=\"detect_button\">DETECT</button>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<div id=\"capture\">";
	    if (capture.model_loaded)
	    	ss << "Model Loaded!<br>";
	    else
	    	ss << "Model Loading...<br>";
	    ss << "<img alt=\"Original Image\" src=\"data:image/png;base64," << capture.original.str() << "\"/><br>";
	    ss << "<img alt=\"Manipulated Image\" src=\"data:image/png;base64," << capture.manipulated.str() << "\"/><br>";
	    ss << "</div>";
	    ss << "<br>";
	    ss << "<a href=\"/capturefactory\">Capture</a>";
	    ss << "<br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
	}

	mg_printf(conn, "</body></html>\n");
	return true;
}

