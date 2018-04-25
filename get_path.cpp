#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

#include "opencv2/opencv.hpp"

// uncomment these to enable various layers of the debug draw
//#define DRAW_CONTOUR_POINTS
#define DRAW_PATH_OUTLINE
//#define DRAW_COMPARISONS
#define DRAW_PATH
#define DRAW_INFO

// BGR format
const cv::Scalar C_RED = cv::Scalar(0, 0, 255);
const cv::Scalar C_GREEN = cv::Scalar(0, 255, 0);
const cv::Scalar C_BLUE = cv::Scalar(255, 0, 0);
const cv::Scalar C_BLACK = cv::Scalar(0, 0, 0);
const cv::Scalar C_WHITE = cv::Scalar(255, 255, 255);
const cv::Scalar C_YELLOW = cv::Scalar(0, 255, 255);
const cv::Scalar C_DARK_GRAY = cv::Scalar(100, 100, 100);
const cv::Scalar C_LIGHT_GRAY = cv::Scalar(180, 180, 180);

const uint16_t FRAME_WIDTH = 640;
const uint16_t FRAME_HEIGHT = 480;

const double PI = 3.141592653589793;
const double PI_180 = 180/PI;
const uint8_t RADIANS = 0;
const uint8_t DEGREES = 1;
class Line2 {
public:
	Line2(cv::Point _p1, cv::Point _p2) : p1(_p1), p2(_p2) {
	}

	static double dist(cv::Point p1, cv::Point p2) {
		return sqrt( pow( (p2.x - p1.x), 2) + pow( (p2.y - p1.y), 2) );
	}
	static double angle(cv::Point p1, cv::Point p2, uint8_t unit = RADIANS, bool half_domain = false) {
		// calculate as radians, returns +-PI
		double a = atan2( (p2.y - p1.y), (p2.x - p1.x) );
		// returns positive only - useful if only want to check angle between lines, not vectors
		if(half_domain) {
			if(a < 0) a += PI;
		}
		// check degrees if specified
		if(unit == DEGREES) a *= PI_180;
		return a;
	}
	static cv::Point midpoint(cv::Point p1, cv::Point p2) {
		return cv::Point( (p1.x + p2.x) / 2 , (p1.y + p2.y) / 2);
	}

	double getAngleLim(uint8_t unit = RADIANS) {
		 return angle(p1, p2, unit, true);
	}

	// accessors just pass the internal points into the static functions
	double getLength() {
		return dist(p1, p2);
	}

	double getAngle(uint8_t unit = RADIANS) {
		// bottom edge of frame is 0;
		return angle(p1, p2, unit);
	}

	static std::string & getType() {
		return type;
	}

	cv::Point & getP1() {
		return p1;
	}
	cv::Point & getP2() {
		return p2;
	}
	cv::Point getMidpoint() {
		return midpoint(p1, p2);
	}

	void swapPoints() {
		cv::Point tmp = p1;
		p1 = p2;
		p2 = tmp;
	}

private:
	cv::Point p1,p2;
	static std::string type;
};

std::string Line2::type = "Line2";

// short to long
bool compareLength(Line2 &first, Line2 &second) {
	if(first.getLength() > second.getLength()) return true;
	return false;
}

int main(int argc, char* argv[]) {
	using namespace std;
	using namespace cv;

	VideoCapture cap;
	cap.open(0);

	if(!cap.isOpened()) {
		CV_Assert("Stream could not be opened.");
	}

	cap.set(CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

	namedWindow("dilate", WINDOW_AUTOSIZE);
	namedWindow("output", WINDOW_AUTOSIZE);
	//moveWindow("output", -800, 0);

	Mat in, frame_hsv, frame_threshold, frame_dilate, out;

	Mat kernel_dilate = getStructuringElement(MORPH_RECT, Size(5, 5));

	while(true) {
		cap.grab();
		cap.retrieve(in);

		string debug_text;

		if(!in.empty()) {
			in.copyTo(out);

			cvtColor(in, frame_hsv, COLOR_BGR2HSV);

			inRange(frame_hsv, Scalar(2, 111, 100), Scalar(18, 255, 255), frame_threshold);
			//inRange(frame_hsv, Scalar(99, 72, 144), Scalar(115, 255, 199), frame_threshold);

			dilate(frame_threshold, frame_dilate, kernel_dilate);

			vector<vector<Point>> contours;

			findContours(frame_dilate, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0,0));

			if(contours.size() > 0) {
				Moments m = moments(contours[0], true);
				int area_max = m.m00;
				int index_max = 0;

				if(contours.size() > 1) {
					for(int i = 1; i < contours.size(); i++) {
						m = moments(contours[i], true); // true b/c binary image
						if(m.m00 > area_max) {
							index_max = i;
							area_max = m.m00;
						}
					}
				}
				debug_text += "Max area: " + to_string(area_max) + "   ";

				// run a polygon approximation on it and let it have a decently high tolerance
				vector<Point> approx_poly_points;
				const double eps = 5;
				approxPolyDP(contours[index_max], approx_poly_points, eps, true);

				// drawContours requires an array of arrays, ie a vector, 
				// so package the one thing we want to draw into a vector
#ifdef DRAW_CONTOUR_POINTS
				vector<vector<Point>> contour_list;
				contour_list.push_back(approx_poly_points);
				drawContours(out, contour_list, -1, C_GREEN, 1, LINE_4);
				// and also draw the points so we can figure out if the lines are long enough
				for(int i = 0; i < contour_list[0].size(); i++) {
					Point p = contour_list[0][i];
					circle(out, p, 2, C_BLUE, 1, FILLED);
				}
#endif
				
				// makes lines out of the points
				list<Line2> boundary_lines;
				int len = approx_poly_points.size();
				for(int i = 0; i < len; i++) {
					if(i+1 == len) {
						// last and first point
						boundary_lines.push_back(Line2(approx_poly_points[i], approx_poly_points[0]));
					} else {
						// this point and next
						boundary_lines.push_back(Line2(approx_poly_points[i], approx_poly_points[i+1]));
					}
				}

				// sort from long to short, long gets first chance as target
				boundary_lines.sort(compareLength);

#ifdef DRAW_PATH_OUTLINE
				// draw each line and its endpoints
				for(list<Line2>::iterator i = boundary_lines.begin(); i != boundary_lines.end(); ++i) {
					// line
					line(out, i->getP1(), i->getP2(), C_YELLOW, 1);
					// circle at second point
					circle(out, i->getP2(), 2, C_BLUE);
				}
#endif
				
				vector<Line2> center_lines;

				// limit execution to number of pairs available or 4, whichever is smaller
				int possible_sections = boundary_lines.size()/2;
				int max_sections = (possible_sections < 4 ? possible_sections : 4);

				// if nothing left or max sections found, stop
				while(!boundary_lines.empty() && center_lines.size() < max_sections) {
					// make copy of target line and remove it from source array
					Line2 target = *(boundary_lines.begin());
					list<Line2>::iterator i = boundary_lines.erase(boundary_lines.begin());

					// this can actually be really large (compared to what one might think of for parallel)
					double parallel_tolerance = 15; // +- degrees

					double t_a = target.getAngleLim(DEGREES);

					vector<list<Line2>::iterator> possible_matches;

					for(; i != boundary_lines.end(); ++i) {
						double angle = i->getAngleLim(DEGREES);
						if(abs(t_a - angle) < parallel_tolerance) {
							possible_matches.push_back(i);
						}
					}

					if(!possible_matches.empty()) {
						// use the first as the default
						int match_index = 0;
						// something large
						int min_dist = 9999999;
						// choose line with nearest midpoint as match
						Point t_mp = target.getMidpoint();
						for(int i = 0; i < possible_matches.size(); i++) {
							double d = Line2::dist(t_mp, possible_matches[i]->getMidpoint());
#ifdef DRAW_COMPARISONS							
							line(out, t_mp, possible_matches[i]->getMidpoint(), C_BLACK);
#endif
							if(d < min_dist) {
								min_dist = d;
								match_index = i;
							}
						}

						Line2 match = *(possible_matches[match_index]);

						// so now we have two lines which are near parallel, of similar lengths, but possibly facing in opposite directions.
						// force the lines to have positive angles, both so that centerlines calculate correctly, and to be predictable
						if(target.getAngle() < 0) target.swapPoints();
						if(match.getAngle() < 0) match.swapPoints();

						// build centerline
						center_lines.push_back( Line2(
							Line2::midpoint(target.getP1(), match.getP1()),
							Line2::midpoint(target.getP2(), match.getP2())
						) );

						// remove match from source
						boundary_lines.erase(possible_matches[match_index]);
					}
				}

				if(!center_lines.empty()) {

					// try to connect lines point to point, returns a set of points ordered from one end of the path to the other
					// preferably bottom-most endpoint first

#ifdef DRAW_PATH
					// draw each line and its endpoint
					debug_text += to_string(center_lines.size());
					for(int i = 0; i < center_lines.size(); i++) {
						// line
						line(out, center_lines[i].getP1(), center_lines[i].getP2(), C_WHITE, 1);
						// circle at each endpoint
						circle(out, center_lines[i].getP1(), 5, C_BLUE);
						circle(out, center_lines[i].getP2(), 5, C_GREEN);
					}
#endif
				}
				
			}
#ifdef DRAW_INFO
			rectangle(out, Point(0,0), Point(500, 20), C_WHITE, CV_FILLED);
			putText(out, debug_text, Point(10,10), FONT_HERSHEY_SIMPLEX, 0.35, C_BLACK, 1);
#endif
			imshow("dilate", frame_dilate);
			imshow("output", out);
		}

		waitKey(100);
	}

	cap.release();
	destroyAllWindows();
}