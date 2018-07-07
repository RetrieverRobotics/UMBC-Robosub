
#include "NamedClass.hpp"

const std::string& NamedClass::getClassName() {
	return class_name;
}
const std::string& NamedClass::getInstanceName() {
	return instance_name;
}
const std::string NamedClass::getFullName() {
	return class_name + "#" + instance_name;
}

NamedClass::NamedClass(std::string _class_name) : class_name(_class_name) {
	std::ostringstream address;
	address << (void const *)this;
	instance_name = address.str();
}

NamedClass::NamedClass(std::string _class_name, std::string _instance_name) : class_name(_class_name), instance_name(_instance_name) {}