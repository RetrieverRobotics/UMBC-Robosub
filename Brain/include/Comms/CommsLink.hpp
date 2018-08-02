#ifndef COMMS_LINK_H
#define COMMS_LINK_H

#include <iostream>
#include <string>
#include <memory>

#include "plog/Log.h"

#include "CommsUtil.hpp"
#include "CommsUtilAccessor.hpp"

/*
CommsLink is a header-only abstract class on which to build links for various
communication interfaces. Each CommsLink must provide a send and receive method.
Each CommsLink also takes a reference to the parent Comms instance, the buffer_map
provided for it, and the name it was given in Comms::addLink().

CommsLink implements the function to set in the buffer and handles the mutex
locking in the process thereby removing such concerns from individual links.

CommsLink is designed around a text-based interface, however there is no reason
one could not also implement i2c or SPI protocols for devices. In that kind of case,
the Link would watch a specific field for instructions, and load received data back
into a different specific field.
*/

class Comms;

// abstract class
class CommsLink {
public:

	CommsLink() {}
	virtual ~CommsLink() {}

	virtual void send(const std::string& field_name, std::shared_ptr<comms_util::DataTS> data) = 0;
	virtual void receive() = 0;

	void init(Comms* _comms, const std::string& _link_id, std::shared_ptr<comms_util::inner_map_t> _buffer) {
		comms = _comms;
		link_id = _link_id;
		buffer = _buffer;
	}

	void typeNotSupported(const std::string& field_name) {
		LOG_WARNING << "The data type used by '" << field_name << "' is not supported by link: " << link_id << ".";
	}

protected:
	std::string link_id;

	template<typename T>
	bool setInBuffer(const std::string& field_name, const comms_util::Hint& type_hint, T field_value);
	
private:
	Comms* comms;
	std::shared_ptr<comms_util::inner_map_t> buffer;
};

template<typename T>
bool CommsLink::setInBuffer(const std::string& field_name, const comms_util::Hint& type_hint, T field_value) {
	if(comms != NULL && buffer != NULL) {
		std::mutex* op_mutex = comms_util::getOpMutexFrom(*comms, link_id, buffer);

		if(op_mutex != NULL) {
			op_mutex->lock();
			buffer->erase(field_name);
			buffer->emplace(field_name, std::make_shared<comms_util::TypedDataTS<T>>(type_hint, field_value));
			op_mutex->unlock();
			
			return true;
		}
	}
	return false;
}

#endif