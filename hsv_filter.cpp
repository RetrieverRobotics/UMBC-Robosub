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

	moveWindow("out", 700, 0);

	Mat frame_in, frame_mid, frame_out;

	int high_1, high_2, high_3;
	high_1 = high_2 = high_3 = 0;

	int low_1, low_2, low_3;
	low_1 = low_2 = low_3 = 0;

	// note display order will not match listed order -_-
	createTrackbar("1L", "out", &low_1, 179);
	createTrackbar("1H", "out", &high_1, 179);
	//createTrackbar("1L", "out", &low_1, 255);
	//createTrackbar("1", "out", &high_1, 255);
	createTrackbar("2L", "out", &low_2, 255);
	createTrackbar("2H", "out", &high_2, 255);
	createTrackbar("3L", "out", &low_3, 255);
	createTrackbar("3H", "out", &high_3, 255);

	for(;;) {
		cap.grab();
		cap.retrieve(frame_in);

		if(!frame_in.empty()) {
			imshow("cam 1", frame_in);

			// convert to HSV
			cvtColor(frame_in, frame_mid, COLOR_BGR2HSV);

			// threshold filter: input, low vals, high vals, output
			inRange(frame_mid, Scalar(low_1, low_2, low_3), Scalar(high_1, high_2, high_3), frame_out);

			// mat, point 1, point 2, color, line thickness or CV_FILLED
			rectangle(frame_out, Point(0, 0), Point(100, 100), Scalar(255,255,255), CV_FILLED);
			
			// mat, text, bottom left location, font, scale, color, line thickness
			putText(frame_out, "1L: " + to_string(low_1), Point(10, 10), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "1H: " + to_string(high_1), Point(10, 25), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "2L: " + to_string(low_2), Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "2H: " + to_string(high_2), Point(10, 55), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "3L: " + to_string(low_3), Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			putText(frame_out, "3H: " + to_string(high_3), Point(10, 85), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);
			
			imshow("out", frame_out);
		}

		waitKey(50);

	}

	cap.release();
	destroyAllWindows();

}