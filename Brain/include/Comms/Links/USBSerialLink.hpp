#ifndef USB_SERIAL_LINK_H
#define USB_SERIAL_LINK_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <memory>

#include "../CommsLink.hpp"
#include "string_util.hpp"

#include <libserialport.h> // c library

/*
USBSerialLink implements a syntax to send typed data over a UART, and
receive by the same syntax.
Template:
~~field_name~type~data
where
	'~' is just a separator
	type will correspond to a comms_util::Hint type
	data is a string, though formatted and parsed according to the Hint associated
		with the 'type'

*/


// std::string::c_str -> const char*

class USBSerialLink : public CommsLink {
public:
	USBSerialLink(const std::string& device_descriptor, int baud_rate);
	~USBSerialLink();

	void send(const std::string& field_name, std::shared_ptr<comms_util::DataTS> data);
	void receive();

	static const std::string stringifyPorts();
private:
	const char separator;
	const char array_separator;

	std::unordered_map<comms_util::Hint, std::string> hint_strings;

	std::string unparsed_input;

	struct sp_port* port;

	template<typename T>
	const std::string format(T data);
	template<typename T>
	const std::string format(std::vector<T> data);

	std::vector<int> aStoiV(const std::string& s, const char arr_sep);
	std::vector<double> aStodV(const std::string& s, const char arr_sep);

	void transmit(const std::string& field_name, const std::string& type_hint, const std::string formatted_data);

	std::string parse(const std::string& parse_me);
	comms_util::Hint deduceHint(const std::string& hint_str);
};

template<typename T>
const std::string USBSerialLink::format(T data) {
	std::ostringstream oss;
	oss << data; // take advantage of conversions with operator<<
	return oss.str();
}

// insert overloads or specializations for container types
template<typename T>
const std::string USBSerialLink::format(std::vector<T> data) {
	std::ostringstream oss;
	for(const auto& elem : data) {
		oss << elem << array_separator;
	}
	return oss.str();
}


#endif