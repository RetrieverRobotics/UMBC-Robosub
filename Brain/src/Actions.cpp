
#include "Actions.hpp"

#include <iostream>
#include <thread>
#include <chrono>

/*

Example Implementation

void action::XXX:step(void) {
	
}

*/

void action::SearchForQualGate::step(void) {
	if(i == 50) dir = "locked";
	++i;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void action::MoveTowardsQualGate::step(void) {
	std::cout << "Moving towards qual gate..." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void action::Interpreter::step(void) {
	std::getline(std::cin, input);
}