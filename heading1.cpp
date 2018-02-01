#include <iostream>

#include "opencv2/opencv.hpp"

// INCOMPLETE

class ContourInfo {
public:
	ContourInfo(std::vector<cv::Point> _contours, cv::Moments _moments, cv::Point2f _center)
		: contours(_contours), moments(_moments), center(_center) {}

	std::vector<cv::Point> getContours() {
		return contours;
	}
	cv::Moments getMoments() {
		return moments;
	}
	cv::Point2f getCenter() {
		return center;
	}

private:
	std::vector<cv::Point> contours;
	cv::Moments moments;
	cv::Point2f center;
};

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	VideoCapture cap;
	cap.open(0);

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

			in.copyTo(out); // could just overwrite in (same image really), but that might confuse me later

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

			if(contours.size() > 0) {
				string debug_text;

				vector<ContourInfo> packs;

				for(int i = 0; i < contours.size(); i++) {
					// find contour moments
					Moments m = moments(contours[i], true); // true for binary image

					// if the area of the contour meets a minimum area requirement
					// set by the GUI slider, move the contour and the resultant moments
					// to a new array

					if(m.m00 > area_lower) {
						Point2f c(m.m10/m.m00, m.m01/m.m00);

						packs.push_back(ContourInfo(contours[i], m, c));
					}
				}

				// TODO: sort useful contours by area
				// may cause incorrect depth -> lock depth with pressure sensor and maintain flat, use only for heading
				// still may have issues with the pin beyond, but worry about that later

				// also, find vertical sections, near ones will have approximately the same top and bottom
				// limited velocity estimation from stereo app?
				
				// hacky but good for now
				for(int i = 0; i < packs.size(); i++) {
					vector<vector<Point>> contour_list;
					contour_list.push_back(packs[i].getContours());
					// mat, list, index of list (or -1=all), color as BGR, line thickness, line type (4,8,AA)
					drawContours(out, contour_list, -1, Scalar(0, 255, 0), 1, LINE_8);
					circle(out, packs[i].getCenter(), 10, Scalar(255, 0, 0), 1, FILLED);
				}

				
				debug_text += to_string(packs.size());

				putText(out, debug_text, Point(10, 10), FONT_HERSHEY_SIMPLEX, 0.35, Scalar(0,0,0), 1);

			}

			// TODO: open a serial port to talk to the microcontroller for the motors and sensors

			imshow("output", out);
		}
		// max 20 fps
		waitKey(500);
	}

	cap.release();
	destroyAllWindows();
}