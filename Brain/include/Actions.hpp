#ifndef ACTIONS_H
#define ACTIONS_H

#include "Threadable.hpp"

/*
This is a library file for commonly used Threadables.
Note the use of the 'actions' namespace.
*/

/*

Example declaration:

class XXX : public Threadable {
public:
	XXX() {}

	void step(void);

private:
}

*/

namespace action {
	class SearchForQualGate : public Threadable {
	public:
		SearchForQualGate() : i(0) {}

		void step(void);

		std::string getDirection(void) { return dir; }

	private:
		int i;
		std::string dir;
	};

	class MoveTowardsQualGate : public Threadable {
	public:
		MoveTowardsQualGate() {}

		void step(void);
	private:
	};

	class Interpreter : public Threadable {
	public:
		Interpreter() {}

		void step(void);

		std::string getStr() { return input; }
	private:
		std::string input;
	};
}



#endif