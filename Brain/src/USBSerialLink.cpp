#include "Comms/Links/USBSerialLink.hpp"

//https://github.com/wjwwood/serial
#include <iostream>

#include "plog/Log.h"

using namespace comms_util;

USBSerialLink::USBSerialLink(const std::string& device_descriptor, int baud_rate) : separator('~'), array_separator(',') {
	sp_return result = sp_get_port_by_name(device_descriptor.c_str(), &port);
	if(result == SP_OK) {
		result = sp_open(port, SP_MODE_READ_WRITE);
		if(result == SP_OK) {
			sp_set_baudrate(port, baud_rate); // doesn't affect a Teensy 3.X
		} else throw std::runtime_error("Error opening serial port: " + device_descriptor);
	} else throw std::runtime_error("Error getting serial port: " + device_descriptor);

	hint_strings.emplace(Hint::Bool, "b");
	hint_strings.emplace(Hint::Int, "i");
	hint_strings.emplace(Hint::IntVector, "i[]");
	hint_strings.emplace(Hint::Double, "d");
	hint_strings.emplace(Hint::DoubleVector, "d[]");
	hint_strings.emplace(Hint::String, "s");
}
USBSerialLink::~USBSerialLink() {
	sp_return result = sp_close(port);
	if(result != SP_OK) throw std::runtime_error("Error closing serial port: " + std::string(sp_get_port_name(port)));
}

void USBSerialLink::send(const std::string& field_name, std::shared_ptr<DataTS> data) {
	// could maybe key an unordered_map with a Hint resulting in a string, keeps it independent of the order
	// then iterate the map, comparing strings and grab the key, then switch on key for behavior
	Hint type_hint = data->getTypeHint();
	try {
		switch(type_hint) {
			case Hint::String:
			{
				std::string s = (std::static_pointer_cast<TypedDataTS<std::string>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(s));
				break;
			}
			case Hint::Bool:
			{
				bool b = (std::static_pointer_cast<TypedDataTS<bool>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(b));
			}
			case Hint::Int:
			{
				int i = (std::static_pointer_cast<TypedDataTS<int>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(i));
				break;
			}
			case Hint::IntVector:
			{
				std::vector<int> vi = (std::static_pointer_cast<TypedDataTS<std::vector<int>>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(vi));
				break;
			}
			case Hint::Double:
			{
				double d = (std::static_pointer_cast<TypedDataTS<double>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(d));
				break;
			}
			case Hint::DoubleVector:
			{
				std::vector<double> vd = (std::static_pointer_cast<TypedDataTS<std::vector<double>>>(data))->getContents();
				transmit(field_name, hint_strings.at(type_hint), format(vd));
				break;
			}
			default:
				typeNotSupported(field_name);
		}
	} catch(std::out_of_range& e) {
		std::cerr << "In USBSerialLink::send(), Hint type not found in hint_strings map." << std::endl;
	}
}

// USB packets are 64 bytes, larger number of bytes per transmit are better
// Teensy will send data faster if it doesn't have to wait for confirmation between data dumps (avoid)

// transmit is blocking only if there is something left in the output buffer from the previous write
void USBSerialLink::transmit(const std::string& field_name, const std::string& type_hint, const std::string formatted_data) {
	std::string line;
	line.append(2, separator);
	line += field_name + separator + type_hint + separator + formatted_data + "\n";

	sp_return result = sp_drain(port); // wait for port to finish transmitting previous if necessary
	if(result != SP_OK) { throw std::runtime_error("Error while attempting to drain output buffer."); }

	const char* str = line.c_str();
	result = sp_nonblocking_write(port, str, strlen(str));
	if(result < 0) { throw std::runtime_error("Error while transmitting."); } // 0 is SP_OK, anything more than that is number of bytes written
}

void USBSerialLink::receive() {
	// read up to n chars from serial port (non blocking)
	int bytes_waiting = sp_input_waiting(port);
	if(bytes_waiting > 0) {
		char buff[bytes_waiting];
		int byte_cnt = sp_nonblocking_read(port, buff, bytes_waiting);
		if(byte_cnt > 0) {
			std::string new_input = std::string(buff, byte_cnt);
			unparsed_input = parse(unparsed_input + new_input);
		}
	}
}

std::string USBSerialLink::parse(const std::string& parse_me) {
	std::string keep_for_later;
	std::vector<std::string> lines;
	string_util::splitOnChar(parse_me, '\n', lines); // removes token from result
	int usable_lines = lines.size();
	if(parse_me[parse_me.length()-1] != '\n') {
		// keep the last line for later, it'll get finished after the the next append
		keep_for_later = lines[lines.size()-1]; // store the last element to return
		--usable_lines; // and then don't use the incomplete line
	}
	if(usable_lines > 0) {
		for(int i = 0; i < usable_lines; ++i) {
			std::string& line = lines[i]; // vector[] returns reference
			line = string_util::trim(line);

			// in substring, verify that first two characters are separator (drop input if not), then remove the first two and split on separator
			if(line.find(std::string(2u, separator)) == 0) {
				line = line.substr(2);
				std::vector<std::string> fields;
				string_util::splitOnChar(line, separator, fields); // modify splitOnChar to limit the number of splits to handle the use of separator in data (though not field_name)
				if(fields.size() == 3) {
					Hint type_hint = deduceHint(fields[1]);
					std::string& data = fields[2];
					switch(type_hint) {
						case Hint::Bool:
							setInBuffer(fields[0], type_hint, (data == "true" ? true : false));
							break;
						case Hint::Int:
							setInBuffer(fields[0], type_hint, std::stoi(data));
							break;
						case Hint::IntVector:
							setInBuffer(fields[0], type_hint, aStoiV(data, array_separator));
							break;
						case Hint::Double:
							setInBuffer(fields[0], type_hint, std::stod(data));
							break;
						case Hint::DoubleVector:
							setInBuffer(fields[0], type_hint, aStodV(data, array_separator));
							break;
						case Hint::String:
							setInBuffer(fields[0], type_hint, data);
							break;
						default:
							typeNotSupported(fields[0]);
					}
				} else continue; 
			} else {
				if(line != "") {
					LOG_INFO << "(USB) " << line;
				}
				continue;
			}
		}
	}
	return keep_for_later;
}

Hint USBSerialLink::deduceHint(const std::string& hint_str) {
	for(auto it = hint_strings.begin(); it != hint_strings.end(); ++it) {
		if(it->second == hint_str) return it->first;
	}
	return Hint::Other;
}

std::vector<int> USBSerialLink::aStoiV(const std::string& s, const char arr_sep) {
	std::vector<std::string> v_tmp;
	string_util::splitOnChar(s, arr_sep, v_tmp);
	std::vector<int> v_final;
	for(int i = 0; i < v_tmp.size(); i++) {
		if(v_tmp[i] != "") v_final.push_back(stoi(v_tmp[i]));
	}
	return v_final;
}

std::vector<double> USBSerialLink::aStodV(const std::string& s, const char arr_sep) {
	std::vector<std::string> v_tmp;
	string_util::splitOnChar(s, arr_sep, v_tmp);
	std::vector<double> v_final;
	for(int i = 0; i < v_tmp.size(); i++) {
		if(v_tmp[i] != "") v_final.push_back(stod(v_tmp[i]));
	}
	return v_final;
}

const std::string USBSerialLink::stringifyPorts() {
	std::string output = "";
	struct sp_port** ports;

	sp_return result = sp_list_ports(&ports);
	if(result == SP_OK) {
		for(int i = 0; ports[i]; ++i) {
			std::string tmp = std::string(sp_get_port_name(ports[i]));
			output += tmp + "\n";
		}
		sp_free_port_list(ports);
	}

	return output;
}