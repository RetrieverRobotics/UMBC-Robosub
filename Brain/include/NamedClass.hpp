#ifndef NAMED_CLASS_H
#define NAMED_CLASS_H

#include <string>
#include <sstream>

class NamedClass {
public:
	const std::string& getClassName();
	const std::string& getInstanceName();
	const std::string getFullName();
	
protected:
	explicit NamedClass(std::string _class_name); // don't use references, b/c then string literal doesn't convert
	NamedClass(std::string _class_name, std::string _instance_name);

	std::string class_name;
	std::string instance_name;
};

#endif