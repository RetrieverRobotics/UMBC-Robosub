#include <iostream>

#include "opencv2/opencv.hpp"

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	VideoCapture cap;
	cap.open(1);

	if(!cap.isOpened()) {
		CV_Assert("Cam 1 open failed");
	}

	cap.set(CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CAP_PROP_FRAME_HEIGHT, 480);

	namedWindow("cam 1", WINDOW_AUTOSIZE);
	namedWindow("out", WINDOW_AUTOSIZE);

	Mat frame_in, frame_hsv, frame_mid, frame_out;

	// setup the Blob Detectors parameters
	SimpleBlobDetector::Params params;

	params.filterByArea = true;
	//slider
	int min_area = 10;
	params.minArea = min_area;
	createTrackbar("Min Area", "out", &min_area, 1000);

	//slider
	int max_area = 100;
	params.maxArea = max_area;
	createTrackbar("Max Area", "out", &max_area, 10000);

	// put these here so I don't forget they exist
	params.filterByCircularity = false;
	params.filterByColor = false;
	params.filterByConvexity = false;
	params.filterByInertia = false;

	Ptr<SimpleBlobDetector> detector; 

	for(;;) {
		cap.grab();
		cap.retrieve(frame_in);

		if(!frame_in.empty()) {
			imshow("cam 1", frame_in);

			cvtColor(frame_in, frame_hsv, COLOR_BGR2HSV);

			inRange(frame_hsv, Scalar(2,111,100), Scalar(18, 255, 255), frame_mid);

			std:vector<KeyPoint> keypoints;

			params.minArea = min_area;
			params.maxArea = max_area;
			detector = SimpleBlobDetector::create(params); // or create(params)
			detector->detect( frame_mid, keypoints );
			drawKeypoints(frame_mid, keypoints, frame_out, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
			
			// mat, point 1, point 2, color, line thickness or CV_FILLED
			rectangle(frame_out, Point(0, 0), Point(100, 50), Scalar(255,255,255), CV_FILLED);
			// mat, text, bottom left location, font, scale, color, line thickness
			putText(frame_out, "Min: " + to_string(min_area), Point(10, 10), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "Max: " + to_string(max_area), Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);

			imshow("out", frame_out);
		}
		// max 20 fps
		waitKey(50);

	}

	cap.release();
	destroyAllWindows();

}