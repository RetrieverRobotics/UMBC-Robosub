#ifndef NAMED_CLASS_H
#define NAMED_CLASS_H

#include <string>
#include <sstream>

/*
NamedClass provides a way to force a "child" class to declare a name for itself.
The use of "class" and "instance" here do not match traditional c++ heirarchies.
Instead, the class name is the name of a base class and instance name is the name
of a derived class. This is particularly applicable to Task and Threadable as
these must be derived to be useful.

This may be deprecated in the next few commits and the naming system changed.
*/

class NamedClass {
public:
	const std::string& getClassName() const;
	const std::string& getInstanceName() const;
	const std::string getFullName() const;
	
protected:
	explicit NamedClass(std::string _class_name); // don't use references, b/c then string literal doesn't convert
	NamedClass(std::string _class_name, std::string _instance_name);

	std::string class_name;
	std::string instance_name;
};

#endif