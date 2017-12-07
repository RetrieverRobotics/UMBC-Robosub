#include <iostream>

#include "opencv2/opencv.hpp"

// INCOMPLETE

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	VideoCapture cap;
	cap.open(1);

	if(!cap.isOpened()) {
		CV_Assert("Stream could not be opened");
	}

	cap.set(CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CAP_PROP_FRAME_HEIGHT, 480);

	namedWindow("threshold", WINDOW_AUTOSIZE);
	namedWindow("output", WINDOW_AUTOSIZE);

	Mat in, frame_hsv, frame_filtered, frame_effect, out;

	Mat kernel_dilate = getStructuringElement(MORPH_RECT, Size(7, 7));

	int area_lower = 0;
	createTrackbar("Area (L)", "output", &area_lower, 500);

	for(;;) {
		// get a new frame from the video stream and decode it
		cap.grab();
		cap.retrieve(in);

		// an empty video frame causes the program to crash
		if(!in.empty()) {

			cvtColor(in, frame_hsv, COLOR_BGR2HSV);

			// apply a threshold filter that outputs a binary image
			// values selected using hsv_filter.cpp
			inRange(frame_hsv, Scalar(2, 111, 100), Scalar(18, 255, 255), frame_filtered);

			// make the white parts bigger
			// not strictly needed, but seems to help a little in lower light
			dilate(frame_filtered, frame_effect, kernel_dilate);

			imshow("threshold", frame_effect);

			vector<vector<Point>> contours;

			// find contours and store them in the contours variable
			// ... RETR_EXTERNAL - store only the outermost contours
			// ... CHAIN_APPROX_SIMPLE - for vertical/horizontal lines, store only the two points at the ends
			// ... Point(X, Y) - offset all contour points when stored, useful if the analyzed region is some
			// portion (Region of Interest), but the contours need to be mapped back onto the original image
			findContours(frame_effect, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));

			vector<Moments> mu(contours.size()); // all moments for each contour

			vector<vector<Point>> useful_contours;
			vector<Moments> useful_moments;
			vector<Point2f> useful_centers;

			for(int i = 0; i < contours.size(); i++) {
				// find contour moments
				mu[i] = moments(contours[i], true); // true for binary image

				// if the area of the contour meets a minimum area requirement
				// set by the GUI slider, move the contour and the resultant moments
				// to a new set of lists
				if(mu[i].m00 > area_lower) {
					useful_contours.push_back(contours[i]);
					useful_moments.push_back(mu[i]);
					useful_centers.push_back( Point2f(mu[i].m10/mu[i].m00, mu[i].m01/mu[i].m00));
				}
			}

			in.copyTo(out); // could just overwrite in (same image really), but that might confuse me later

			// then only draw the contours that meet the area requirement
			// mat, list, index of list (or -1=all), color as BGR, line thickness, line type (4,8,AA)
			drawContours(out, useful_contours, -1, Scalar(0, 255, 0), 1, LINE_8);
			for(int i = 0; i < useful_contours.size(); i++) {
				// mat, point, radius, color, thickness, line_type
				circle(out, useful_centers[i], 10, Scalar(255, 0, 0), 1, FILLED);
			}

			imshow("output", out);
		}
		// max 20 fps
		waitKey(50);
	}

	cap.release();
	destroyAllWindows();
}