#include <iostream>
#include <algorithm>

#include "opencv2/opencv.hpp"

// BGR format
#define h1_RED Scalar(0, 0, 255)
#define h1_GREEN Scalar(0, 255, 0)
#define h1_BLUE Scalar(255, 0, 0)
#define h1_BLACK Scalar(0, 0, 0)
#define h1_WHITE Scalar(255, 255, 255)
#define h1_DARK_GRAY Scalar(100, 100, 100)
#define h1_LIGHT_GRAY Scalar(180, 180, 180)

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

// INCOMPLETE

class ContourInfo {
public:
	ContourInfo(std::vector<cv::Point> _contour, cv::Moments _moments)
		: contour(_contour), moments(_moments) {
			// in, out, epsilon, closed
			cv::approxPolyDP(contour, approx_poly, 2, true);
			center = cv::Point2f(moments.m10/moments.m00, moments.m01/moments.m00);
			bounding_box = cv::boundingRect(approx_poly);
			min_bounding_box = cv::minAreaRect(approx_poly);
	}

	std::vector<cv::Point> getContour() {
		return contour;
	}
	std::vector<cv::Point> getPoly() {
		return approx_poly;
	}
	cv::Moments getMoments() {
		return moments;
	}
	cv::Point2f getCenter() {
		return center;
	}
	double getArea() {
		return moments.m00;
	}
	cv::Rect getBoundingBox() {
		return bounding_box;
	}
	cv::RotatedRect getMinBoundingBox() {
		return min_bounding_box;
	}

private:
	std::vector<cv::Point> contour;
	std::vector<cv::Point> approx_poly;
	cv::Moments moments;
	cv::Point2f center;
	cv::Rect bounding_box;
	cv::RotatedRect min_bounding_box;
};

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	VideoCapture cap;
	cap.open(0);

	if(!cap.isOpened()) {
		CV_Assert("Stream could not be opened");
	}

	cap.set(CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

	namedWindow("threshold", WINDOW_AUTOSIZE);
	namedWindow("output", WINDOW_AUTOSIZE);

	Mat in, frame_hsv, frame_filtered, frame_effect, out;

	Mat kernel_dilate = getStructuringElement(MORPH_RECT, Size(7, 7));

	int area_lower = 0;
	createTrackbar("Area (L)", "output", &area_lower, 500);

	string debug_text;

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
				vector<ContourInfo> packs;

				for(int i = 0; i < contours.size(); i++) {
					// find contour moments
					Moments m = moments(contours[i], true); // true for binary image

					// if the area of the contour meets a minimum area requirement
					// set by the GUI slider, save the contour for later processing

					if(m.m00 > area_lower) {
						packs.push_back(ContourInfo(contours[i], m));
					}
				}

				if(packs.size() > 0) {
					sort(packs.begin(), packs.end(),
						[](ContourInfo lhs, ContourInfo rhs) {
							// sort largest to smallest
							return lhs.getArea() > rhs.getArea();
						}
					);

					Point target_center(FRAME_WIDTH/2, FRAME_HEIGHT/2);
					// render the contours, hacky but good for now
					for(int i = 0; i < packs.size(); i++) {
						// only draw the first 2
						if(i == 2) break;
						vector<vector<Point>> contour_list;
						contour_list.push_back(packs[i].getPoly());

						drawContours(out, contour_list, -1, h1_GREEN, 1, LINE_AA);
						
						// only for largest
						if(i == 0) {				
							RotatedRect min_bounds = packs[i].getMinBoundingBox();
							Point2f min_bounds_points[4];
							min_bounds.points(min_bounds_points);
							for(int j = 0; j < 4; j++) {
								line(out, min_bounds_points[j], min_bounds_points[(j+1)%4], h1_RED, 1, LINE_8);
							}
							circle(out, min_bounds.center, 5, h1_RED, 1, FILLED);
							target_center = min_bounds.center;
						}
					}

					int tolerance = 7;
					int half_width = FRAME_WIDTH/2;
					int half_height = FRAME_HEIGHT/2;
					int eighth_height = FRAME_HEIGHT/8;

					// cross hairs
					line(out, Point(0, half_height), Point(FRAME_WIDTH, half_height), h1_WHITE, 1, LINE_4);
					line(out, Point(half_width, 0), Point(half_width, FRAME_HEIGHT), h1_WHITE, 1, LINE_4);
					
					// tolerance lines
					int tol_top = half_height - eighth_height;
					int tol_bot = half_height + eighth_height;
					int tol_left = half_width - tolerance;
					int tol_right = half_width + tolerance;

					line(out, Point(tol_left, tol_top), Point(tol_left, tol_bot), h1_WHITE, 1, LINE_4);
					line(out, Point(tol_right, tol_top), Point(tol_right, tol_bot), h1_WHITE, 1, LINE_4);

					//
					int t_x = target_center.x;

					
					int diff = t_x - FRAME_WIDTH/2;
					if(abs(diff) > tolerance) {
						// heading
						line(out, Point(t_x, 0), Point(t_x, FRAME_HEIGHT), h1_RED, 2, LINE_4);
						if(diff > 0) {
							debug_text = "heading to the right";
						} else {
							debug_text = "heading to the left";
						}
					} else {
						debug_text = "on target";
					}
				} else {
					debug_text = "NO TARGET";
				}
			}

			putText(out, debug_text, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,0,0), 2);


			imshow("output", out);
		}
		waitKey(100);
	}

	cap.release();
	destroyAllWindows();
}