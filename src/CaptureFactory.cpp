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

class mythread : public std::thread
{
  public:
    mythread() {}
    static void setScheduling(std::thread &th, int policy, int priority) {
        sched_param sch_params;
        sch_params.sched_priority = priority;
        if(pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
            std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
        }
    }
  private:
    sched_param sch_params;
};

static cv::Rect dlibRectangleToOpenCV(dlib::rectangle r)
{
  return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right() + 1, r.bottom() + 1));
}

static dlib::rectangle openCVRectToDlib(cv::Rect r)
{
  return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x - 1, (long)r.br().y - 1);
}

static std::stringstream matToBase64PNG(cv::Mat* input)
{
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

    std::stringstream png;
    png << encoded.str();
    return png;
}

static std::stringstream matToJPG(cv::Mat* input)
{
    std::vector<uchar> buf;
    try
    {
    	cv::imencode(".jpeg", *input, buf, std::vector<int>() );
    }
    catch( cv::Exception& e )
    {
        const char* err_msg = e.what();
        std::cout << "exception caught: " << err_msg << std::endl;
    }

    stringstream incoming;
    copy(buf.begin(), buf.end(),
         ostream_iterator<uchar>(incoming));

    //incoming << FILE.rdbuf();
    //std::ofstream fout("test.jpg");

    //fout << incoming.rdbuf();

    std::stringstream jpg;
    jpg << incoming.str();
    return jpg;
}

// Apply affine transform calculated using srcTri and dstTri to src
void applyAffineTransform(Mat &warpImage, Mat &src, std::vector<Point2f> &srcTri, std::vector<Point2f> &dstTri)
{
    // Given a pair of triangles, find the affine transform.
    Mat warpMat = getAffineTransform( srcTri, dstTri );

    // Apply the Affine Transform just found to the src image
    warpAffine( src, warpImage, warpMat, warpImage.size(), INTER_LINEAR, BORDER_REFLECT_101);
}

// Calculate Delaunay triangles for set of points
// Returns the vector of indices of 3 points for each triangle
static void calculateDelaunayTriangles(Rect rect, std::vector<cv::Point2f> &points, std::vector< std::vector<int> > &delaunayTri){

	// Create an instance of Subdiv2D
    Subdiv2D subdiv(rect);

	// Insert points into subdiv
    for( std::vector<cv::Point2f>::iterator it = points.begin(); it != points.end(); it++)
        subdiv.insert(*it);

	std::vector<Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);
	std::vector<cv::Point2f> pt(3);
	std::vector<int> ind(3);

	for( size_t i = 0; i < triangleList.size(); i++ )
	{
		Vec6f t = triangleList[i];
		pt[0] = cv::Point2f(t[0], t[1]);
		pt[1] = cv::Point2f(t[2], t[3]);
		pt[2] = cv::Point2f(t[4], t[5 ]);

		if ( rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])){
			for(int j = 0; j < 3; j++)
				for(size_t k = 0; k < points.size(); k++)
					if(abs(pt[j].x - points[k].x) < 1.0 && abs(pt[j].y - points[k].y) < 1)
						ind[j] = k;

			delaunayTri.push_back(ind);
		}
	}

}

std::vector <cv::Point2f> get_points(const dlib::full_object_detection& d)
{
    std::vector <cv::Point2f> points;
    for (int i = 0; i < 68; ++i)
    {
    	points.push_back(cv::Point2f(d.part(i).x(), d.part(i).y()));
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

void draw_polyline(cv::Mat &img, std::vector<cv::Point2f> points, const int start, const int end, bool isClosed = false)
{
    std::vector <cv::Point> points_part;
    for (int i = start; i <= end; ++i)
    {
        points_part.push_back(points[i]);
    }
    cv::polylines(img, points_part, isClosed, cv::Scalar(255,0,0), 2, 16);
}

// Warps and alpha blends triangular regions from img1 and img2 to img
void warpTriangle(Mat &img1, Mat &img2, std::vector<Point2f> &t1, std::vector<Point2f> &t2)
{

    Rect r1 = boundingRect(t1);
    Rect r2 = boundingRect(t2);

    // Offset points by left top corner of the respective rectangles
    std::vector<Point2f> t1Rect, t2Rect;
    std::vector<Point> t2RectInt;
    for(int i = 0; i < 3; i++)
    {

        t1Rect.push_back( Point2f( t1[i].x - r1.x, t1[i].y -  r1.y) );
        t2Rect.push_back( Point2f( t2[i].x - r2.x, t2[i].y - r2.y) );
        t2RectInt.push_back( Point(t2[i].x - r2.x, t2[i].y - r2.y) ); // for fillConvexPoly

    }

    // Get mask by filling triangle
    Mat mask = Mat::zeros(r2.height, r2.width, CV_32FC3);
    fillConvexPoly(mask, t2RectInt, Scalar(1.0, 1.0, 1.0), 16, 0);

    // Apply warpImage to small rectangular patches
    Mat img1Rect;
    img1(r1).copyTo(img1Rect);

    Mat img2Rect = Mat::zeros(r2.height, r2.width, img1Rect.type());

    applyAffineTransform(img2Rect, img1Rect, t1Rect, t2Rect);

    multiply(img2Rect,mask, img2Rect);
    multiply(img2(r2), Scalar(1.0,1.0,1.0) - mask, img2(r2));
    img2(r2) = img2(r2) + img2Rect;


}


static std::stringstream drawToJPG(cv::Mat* input, std::vector<std::vector<cv::Point2f>>* points)
{
	cv::Mat output = (*input).clone();

	//Read points
    for (unsigned long i = 0; i < (*points).size(); ++i)
    {
        draw_polyline(output, (*points)[i], 0, 16);           // Jaw line
        draw_polyline(output, (*points)[i], 17, 21);          // Left eyebrow
        draw_polyline(output, (*points)[i], 22, 26);          // Right eyebrow
        draw_polyline(output, (*points)[i], 27, 30);          // Nose bridge
        draw_polyline(output, (*points)[i], 30, 35, true);    // Lower nose
        draw_polyline(output, (*points)[i], 36, 41, true);    // Left eye
        draw_polyline(output, (*points)[i], 42, 47, true);    // Right Eye
        draw_polyline(output, (*points)[i], 48, 59, true);    // Outer lip
        draw_polyline(output, (*points)[i], 60, 67, true);    // Inner lip
        draw_polyline(output, (*points)[i], 68, 70);          // Forehead

        cv::circle(output, (*points)[i][8], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][29], 3, cv::Scalar(0,255,255), -1, 16);
        cv::circle(output, (*points)[i][69], 3, cv::Scalar(0,255,255), -1, 16);

    }

    return matToJPG(&output);
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
	this->cap = new cv::VideoCapture();

	camMat = new std::vector<cv::Mat>();
	fileMat = new std::vector<cv::Mat>();
	camPoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
	filePoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();

	cv::Mat boodschap(1280,960,CV_8UC3,cv::Scalar(255,255,255));
	cv::putText(boodschap, "Gezichtsherkenningsmodel wordt geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
	pose_model = new dlib::shape_predictor();
	detector = new dlib::frontal_face_detector();

	std::thread t1( [this] { loadModel(); } );
	mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();

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
	this->cap = new cv::VideoCapture();

	camMat = new std::vector<cv::Mat>();
	fileMat = new std::vector<cv::Mat>();
	camPoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();
	filePoints = new std::vector<std::vector<std::vector<cv::Point2f>>>();

	pose_model = new dlib::shape_predictor();
	detector = new dlib::frontal_face_detector();

	std::thread t1( [this] { loadModel(); } );
	mythread::setScheduling(t1, SCHED_IDLE, 0);

	t1.detach();

	std::stringstream ss;
	ss << "/capture-" << this->getUuid();
	url = ss.str().c_str();
	server->addHandler(url, mh);
}

CaptureFactory::Capture::~Capture(){
	delete mh;
	delete cap;
	delete camMat;
	delete fileMat;
	delete camPoints;
	delete filePoints;
	delete pose_model;
	delete detector;
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

void CaptureFactory::Capture::openCap(captureType type)
{
	if (type == CAP_CAM)
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(0);
			cap->set(CAP_PROP_FOURCC ,CV_FOURCC('M', 'J', 'P', 'G') );
			cap->set(CAP_PROP_FRAME_WIDTH,1280);   // width pixels
			cap->set(CAP_PROP_FRAME_HEIGHT,960);   // height pixels
		}
	}
	else
	{
		if(!cap->isOpened()){   // connect to the camera
			cap->open(filmpje);
		}
	}
}

void CaptureFactory::Capture::closeCap(){
	if(cap->isOpened()){   // connect to the camera
		cap->release();
    }

}

void CaptureFactory::Capture::loadModel(){
	delete detector;
	this->detector = new dlib::frontal_face_detector(get_frontal_face_detector());
    cout << "Reading in shape predictor..." << endl;
    deserialize("shape_predictor_68_face_landmarks.dat") >> *pose_model;
    cout << "Done reading in shape predictor ..." << endl;
	cv::Mat boodschap(1280,960,CV_8UC3,cv::Scalar(255,255,255));
	cv::putText(boodschap, "Gezichtsherkenningsmodel geladen!", Point2f(100,100), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
	this->manipulated.str("");
	this->manipulated.clear();
	this->manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&boodschap).str() << "\r\n--frame\r\n";
    model_loaded = true;
}

cv::Mat CaptureFactory::Capture::captureFrame(){
    // Serialize the input image to a stringstream
	cout << "Grabbing a frame..." << endl;
    // Grab a frame
	cv::Mat input;
	try
	{
		*cap >> input;
	}
	catch( cv::Exception& e )
	{
	    const char* err_msg = e.what();
	    std::cout << "exception caught: " << err_msg << std::endl;
	}

    //cap->release();
	return input;
    /*
     * <img alt="Embedded Image" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADIA..." />
     */
}

std::vector<std::vector<cv::Point2f>> CaptureFactory::Capture::detectFrame(cv::Mat* input)
{
       std::vector<std::vector<cv::Point2f>> points;
       cout << "Image size: " << (*input).cols << " x " << (*input).rows << std::endl;
       cv::Mat im_small;
       // Resize image for face detection
       try
       {
        cv::resize(*input, im_small, cv::Size(), 1.0/FACE_DOWNSAMPLE_RATIO, 1.0/FACE_DOWNSAMPLE_RATIO);
       }
       catch( cv::Exception& e )
       {
  	    const char* err_msg = e.what();
        std::cout << "exception caught in resize: " << err_msg << std::endl;
        return points;
       }

       std::vector<dlib::rectangle> faces;
       cout << "Detecting faces..." << endl;
       // Detect faces
       cv_image<bgr_pixel> img(*input);
       cv_image<bgr_pixel> cimg(im_small);
       faces = (*detector)(cimg);
       if (faces.size() == 0)
       {
    	 cout << "No faces detected." << endl;
         return points;
       }
       // Find the pose of each face.
       std::vector<full_object_detection> shapes;
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
    	shape = (*pose_model)(img, r);
        //shapes.push_back(pose_model(cimg, faces[i]));
        shapes.push_back(shape);

       }

       //Read points
       for (unsigned long i = 0; i < faces.size(); ++i)
       {
    	   points.push_back(get_points(shapes[i]));
           //calculate forehead left (point index 68)
           points[i].push_back(cv::Point2f(points[i][17].x+(points[i][17].x-points[i][2].x),points[i][17].y - (points[i][2].y-points[i][17].y)));

           //calculate forehead middle (point index 69)
           points[i].push_back(cv::Point2f(points[i][27].x+(points[i][29].x-points[i][8].x), points[i][29].y - (points[i][8].y - points[i][29].y)));

           //calculate forehead right ((point index 70)
           points[i].push_back(cv::Point2f(points[i][26].x+(points[i][26].x-points[i][14].x),points[i][26].y - (points[i][14].y-points[i][26].y)));
       }

       return points;
}


std::vector<cv::Mat> CaptureFactory::Capture::mergeFrames()
{
	std::vector<cv::Mat> totaal;
	Mat img_cam;
    Mat img_file;
    Mat resultaat;

	for (int frame = 0; frame < (*filePoints).size(); frame++)
	{
		img_file = (*fileMat)[frame].clone();
		img_cam = (*camMat)[frame % ((*camMat).size())].clone();
		resultaat = img_file.clone();
		for (int gezicht = 0; gezicht < (*filePoints)[frame].size(); gezicht++)
		{
	        //convert Mat to float data type
	        img_cam.convertTo(img_cam, CV_32F);
	        resultaat.convertTo(resultaat, CV_32F);

	        // Find convex hull
	        std::vector<Point2f> hull1;
	        std::vector<Point2f> hull2;
	        std::vector<int> hullIndex;

	        cout << "Finding convex hull." << endl;

	        try
	        {
	         convexHull((*filePoints)[frame][gezicht], hullIndex, false, false);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in convexHull: " << err_msg << std::endl;
	      	}

	        for(int i = 0; i < (int)hullIndex.size(); i++)
	        {
	            hull1.push_back((*camPoints)[frame % ((*camPoints).size())][gezicht][hullIndex[i]]);
	            hull2.push_back((*filePoints)[frame][gezicht][hullIndex[i]]);
	        }

	        cout << "Finding delaunay triangulation." << endl;

	        // Find delaunay triangulation for points on the convex hull
	        std::vector< std::vector<int> > dt;
	        Rect rect(0, 0, resultaat.cols, resultaat.rows);
	        try
	        {
	         calculateDelaunayTriangles(rect, hull2, dt);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in calculateDelauneyTriangles: " << err_msg << std::endl;
	      	}


	        cout << "Applying affine transformation." << endl;

	        // Apply affine transformation to Delaunay triangles
	        for(size_t i = 0; i < dt.size(); i++)
	        {
	           std::vector<Point2f> t1, t2;
	           // Get points for img1, img2 corresponding to the triangles
	           for(size_t j = 0; j < 3; j++)
	           {
	        	  t1.push_back(hull1[dt[i][j]]);
	        	  t2.push_back(hull2[dt[i][j]]);
	        	}
	           try
	           {
                warpTriangle(img_cam, resultaat, t1, t2);
	           }
	           catch( cv::Exception& e )
		       {
		   	    const char* err_msg = e.what();
		        std::cout << "exception caught in warpTriangle: " << err_msg << std::endl;
		       }
	       	}

	        resultaat.convertTo(resultaat, CV_8UC3);

	        cout << "Calculating mask." << endl;

	        // Calculate mask
	        std::vector<Point> hull8U;
	        for(int i = 0; i < (int)hull2.size(); i++)
	        {
	            Point pt(hull2[i].x, hull2[i].y);
	            hull8U.push_back(pt);
	        }

	        cout << "Fill Convex Poly." << endl;
	        Mat mask = Mat::zeros(img_file.rows, img_file.cols, img_file.depth());
	        try
	        {
	         fillConvexPoly(mask,&hull8U[0], hull8U.size(), Scalar(255,255,255));
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in fillConvexPoly: " << err_msg << std::endl;
	      	}

	        // Clone seamlessly.
	        Rect r = boundingRect(hull2);
	        //Point center = (r.tl() + r.br()) / 2;
	        Point centertest = Point(img_file(r).cols / 2,img_file(r).rows / 2);

	        cout << "Seamlessclone." << endl;

	        cv::Mat imgtest1, imgtest2, masktest, output;
            imgtest1 = img_file(r);
	        imgtest2 = resultaat(r);
	        masktest = mask(r);
	        try
	        {
	         cv::seamlessClone(imgtest2,imgtest1, masktest, centertest, output, NORMAL_CLONE);
	        }
            catch( cv::Exception& e )
	      	{
	   	     const char* err_msg = e.what();
	         std::cout << "exception caught in seamlessClone: " << err_msg << std::endl;
	      	}

	        cout << "Copy to output." << endl;
			output.copyTo(resultaat(r));
		}
		 cout << "Push back." << endl;
		totaal.push_back(resultaat);
	}
	return totaal;
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
       ss << "<a href=\"/capturefactory\">Captures</a>";
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
		ss << "<h2>Beschikbare Captures:</h2>";
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
	std::string value;
	if(CivetServer::getParam(conn, "streaming", dummy))
	{
		mg_printf(conn,
			          "HTTP/1.1 200 OK\r\nContent-Type: "
			          "multipart/x-mixed-replace; boundary=frame\r\n\r\n");
		std::stringstream ss;
		do {
			ss << capture.manipulated.str();
			ss.seekp(0, ios::end);
			stringstream::pos_type offset = ss.tellp();
			ss.seekp(0, ios::beg);
			mg_write(conn, ss.str().c_str(), offset);
			delay(200);
		} while (true);
	}
	else
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
	    //ss << "<img alt=\"Manipulated Image\" src=\"data:image/png;base64," << capture.manipulated->str() << "\"/><br>";
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

	   std::stringstream ss;
	   ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   mg_printf(conn, ss.str().c_str());
	   mg_printf(conn, "<h2>Wijzigingen opgeslagen...!</h2>");
	}
	else if(CivetServer::getParam(conn, "merge", dummy))
	{
		std::vector<cv::Mat> merged;
		capture.manipulated.str("");
		capture.manipulated.clear();
		merged = capture.mergeFrames();
		for (int i = 0; i < merged.size(); ++i)
		{
			capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&merged[i]).str() << "\r\n--frame\r\n";
		}

		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Merged!...!</h2>");
	}
	else if(CivetServer::getParam(conn, "newselect", value))
	{
		capture.fileMat->clear();
		capture.filePoints->clear();
		capture.filmpje = value;
		std::stringstream ss;
		capture.openCap(CAP_FILE);
		cv::Mat frame;
		for (;;)
		{
			frame = capture.captureFrame();
			if (frame.empty()) break;
			capture.fileMat->push_back(capture.captureFrame());
		}
		capture.closeCap();

		ss << "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" << capture.getUrl() << "\"/></head><body>";
		mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}
	else if(CivetServer::getParam(conn, "new", dummy))
	{
       mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
	   DIR *dirp;
	   struct dirent *dp;
	   std::stringstream ss;
	   ss << "<h2>Selecteer een filmpje:</h2>";
	   ss << "<form action=\"" << capture.getUrl() << "\" method=\"POST\">";
	   if ((dirp = opendir(RESOURCES_DIR)) == NULL) {
	          fprintf(stderr,"couldn't open %s.\n",RESOURCES_DIR);
	   }
       do {
	      errno = 0;
	      if ((dp = readdir(dirp)) != NULL) {
	    	 /*
	    	  * ignore . and ..
	    	  */
	    	if (std::strcmp(dp->d_name, ".") == 0) continue;
	    	if (std::strcmp(dp->d_name, "..") == 0) continue;
	    	ss << "<button type=\"submit\" name=\"newselect\" value=\"" << RESOURCES_DIR << dp->d_name << "\" ";
	    	ss << "id=\"newselect\">Selecteren</button>&nbsp;";
	    	ss << "&nbsp;" << dp->d_name << "<br>";
	        }
	   } while (dp != NULL);
       ss << "<button type=\"submit\" name=\"annuleren\" value=\"annuleren\" id=\"annuleren\">Annuleren</button>&nbsp;";
       ss << "</form>";
       (void) closedir(dirp);
       ss <<  "<br>";
       ss << "<a href=\"/capturefactory\">Captures</a>";
       ss <<  "<br>";
       ss << "<a href=\"/\">Home</a>";
       mg_printf(conn, ss.str().c_str());
       mg_printf(conn, "</body></html>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "save_video", dummy))
	{
		Size S = Size((*capture.camMat)[0].cols,(*capture.camMat)[0].rows);
		int codec = CV_FOURCC('M', 'J', 'P', 'G');
		VideoWriter outputVideo("resources/capture.avi", codec, 5.0, S, true);

		if (outputVideo.open("resources/capture.avi", codec, 5.0, S, true))
			cout << "Video open success!" << endl;
		else
			cout << "Video open failure!" << endl;
		for (int i = 0; i < (*capture.camMat).size(); ++i)
		{
			outputVideo << (*capture.camMat)[i];
		}
		outputVideo.release();
		std::stringstream ss;
		//	ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Video opgeslagen...!</h2>");
	}
	/* if parameter start is present start button was pushed */
	else if(CivetServer::getParam(conn, "capture", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.camPoints->clear();
		capture.camMat->clear();
		capture.camMat->push_back(capture.captureFrame());
		capture.closeCap();
		capture.manipulated.str("");
		capture.manipulated.clear();
		for (int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			/* add points of found faces */
			capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&mat).str() << "\r\n--frame\r\n";
		}
		std::stringstream ss;
	//	ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Nieuwe Capture...!</h2>");
	}
	else if(CivetServer::getParam(conn, "capture_multi", value))
	{
		capture.openCap(CAP_CAM);
		capture.camPoints->clear();
		capture.camMat->clear();
		for (int i = 0; i < atoi(value.c_str()); ++i)
		{
			capture.camMat->push_back(capture.captureFrame());
		}
		capture.closeCap();
		capture.manipulated.str("");
		capture.manipulated.clear();
		for (int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			/* add points of found faces */
			capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&mat).str() << "\r\n--frame\r\n";
		}
		std::stringstream ss;
	//	ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Frames Captured...!</h2>");
	}
	else if(CivetServer::getParam(conn, "capture_next", dummy))
	{
		capture.openCap(CAP_CAM);
		capture.camMat->push_back(capture.captureFrame());
		capture.closeCap();
		capture.manipulated.str("");
		capture.manipulated.clear();
		for (int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			/* add points of found faces */
			capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << matToJPG(&mat).str() << "\r\n--frame\r\n";
		}
		std::stringstream ss;
	//	ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Frame Toegevoegd...!</h2>");
	}
	/* if parameter stop is present stop button was pushed */
	else if(CivetServer::getParam(conn, "detect", dummy))
	{
		std::thread t1( [this] {

		std::vector<std::vector<cv::Point2f>> points;
		capture.manipulated.str("");
		capture.manipulated.clear();
		for (int i = 0; i < capture.camMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.camMat)[i];
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&mat);
			if (points.size() == 0)
			{
				/* remove frame with no detected faces */
				(*capture.camMat).erase((*capture.camMat).begin() + i);
			}
			else
			{
				/* add points of found faces */
				capture.camPoints->push_back(points);
				capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << drawToJPG(&mat, &points).str() << "\r\n--frame\r\n";
			}
		}

		} );

		mythread::setScheduling(t1, SCHED_IDLE, 0);

		t1.detach();

		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Gezichtsdetectie op achtergrond...!</h2>");
	}
	else if(CivetServer::getParam(conn, "detect_file", dummy))
	{
		std::thread t1( [this] {

		std::vector<std::vector<cv::Point2f>> points;
		capture.manipulated.str("");
		capture.manipulated.clear();
		for (int i = 0; i < capture.fileMat->size(); ++i)
		{
			cv::Mat mat  = (*capture.fileMat)[i];
			std::vector<std::vector<cv::Point2f>> points = capture.detectFrame(&mat);
			if (points.size() == 0)
			{
				/* remove frame with no detected faces */
				(*capture.fileMat).erase((*capture.fileMat).begin() + i);
			}
			else
			{
				capture.filePoints->push_back(points);
				capture.manipulated << "Content-Type: image/jpeg\r\n\r\n" << drawToJPG(&mat, &points).str() << "\r\n--frame\r\n";
			}
		}

		} );

		mythread::setScheduling(t1, SCHED_IDLE, 0);

		t1.detach();

		std::stringstream ss;
		ss << "<html><head><meta http-equiv=\"refresh\" content=\"1;url=\"" << capture.getUrl() << "\"/></head><body>";
	   	mg_printf(conn, ss.str().c_str());
	   	mg_printf(conn, "<h2>Gezichtsdetectie op achtergrond...!</h2>");
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
		/*
		ss << "<script type=\"text/javascript\">";
		   ss << " $(document).ready(function(){";
		   ss << "  setInterval(function(){";
		   ss << "  $.get( \"" << capture.getUrl() << "?running=true\", function( data ) {";
		   ss << "  $( \"#capture\" ).html( data );";
		   ss << " });},1000)";
		   ss << "});";
		ss << "</script>";
		*/
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
	    ss << "<button type=\"submit\" name=\"capture\" value=\"capture\" id=\"capture_button\">Capture</button>";
	    ss << "<button type=\"submit\" name=\"capture_next\" value=\"capture_next\" id=\"capture_button_next\">Volgende Frame</button><br>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"30\" id=\"capture_button_30\">Capture 30</button>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"60\" id=\"capture_button_60\">Capture 60</button>";
	    ss << "<button type=\"submit\" name=\"capture_multi\" value=\"120\" id=\"capture_button_120\">Capture 120</button></br>";
	    ss << "<button type=\"submit\" name=\"save_video\" value=\"save_video\" id=\"save_video\">Save as Video</button></br>";
	    ss << "<button type=\"submit\" name=\"detect\" value=\"detect\" id=\"detect_button\">Detect</button>";
	    ss << "<button type=\"submit\" name=\"detect_file\" value=\"detect_file\" id=\"detect__file_button\">Detect File</button>";
	    ss << "<button type=\"submit\" name=\"new\" id=\"new\">Filmpje</button>";
	    ss << "<button type=\"submit\" name=\"merge\" id=\"merge\">Merge</button>";
	    ss << "<button type=\"submit\" name=\"submit\" value=\"submit\" id=\"submit\">Submit</button></br>";
	    ss <<  "</br>";
	    ss << "<div id=\"capture\">";
	    ss << "<img src=\"" << capture.url << "/?streaming=true\">";
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

