blob.t:
	g++ --std=c++11 `pkg-config --cflags --libs opencv` -o bin/blob blob.cpp

hsv_filter.t:
	g++ --std=c++11 `pkg-config --cflags --libs opencv` -o bin/hsv_filter hsv_filter.cpp

contour_moments.t:
	g++ --std=c++11 `pkg-config --cflags --libs opencv` -o bin/contour_moments contour_moments.cpp

heading1.t:
	g++ --std=c++11 `pkg-config --cflags --libs opencv` -o bin/heading1 heading1.cpp

get_path.t:
	g++ --std=c++11 `pkg-config --cflags --libs opencv` -o bin/get_path get_path.cpp