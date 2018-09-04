#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

int main( ) {
	using namespace std;
	using namespace cv;
	
	//Initiate camera
	VideoCapture cap;
	cap.open(0);
	Mat frame_in;
	
	if (!cap.isOpened()) {
		CV_Assert("Cam 1 open failed");
	}
	
	cap.set(CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CAP_PROP_FRAME_HEIGHT, 480);
	
	//Run camera
	while(true) {
		cap.grab();
		cap.retrieve(frame_in);

		if(!frame_in.empty()) {
			imshow("cam 1", frame_in);
		}

		// Load Face cascade (.xml file)
		CascadeClassifier face_cascade;
		face_cascade.load( "PATH_TO_OPENCV/data/haarcascades/haarcascade_frontalface_default.xml" );

		// Detect faces
		std::vector<Rect> faces;
		face_cascade.detectMultiScale(frame_in, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30));

		// Draw circles on the detected faces
		for( int i = 0; i < faces.size(); i++ ) {
			Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
			ellipse( frame_in, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );
		}
		
		//Show results
		imshow( "Face Detection", frame_in );
		waitKey(50);
	}
	return 0;
}
