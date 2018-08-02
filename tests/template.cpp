
#include <iostream>
#include <string>
#include <vector>

// in simple use of defining a template typename and class are interchangeable

template<typename T>
T getTwice(T data) {
	return data*2;
}

template<class T>
T add(T n1, T n2) {
	return n1+n2;
}

template<class T>
double getAverage(T arr[], int cnt) {
	T sum = T(); // call default contsructor - still useful for primitive types

	for(int i = 0; i < cnt; ++i) {
		sum += arr[i];
	}

	return (double)(sum) / cnt;
}

template<class T>
void replaceWithDouble(T& val) {
	val *= 2;
}



int main(int argc, char** argv) {
	{
		int int_arr[] = { 3, 5, 7 };
		int int_cnt = 3;
		std::cout << getAverage(int_arr, int_cnt) << std::endl;
	}

	{
		int val = 30;
		replaceWithDouble(val);
		std::cout << val << std::endl;
	}
	

	return 0;
}