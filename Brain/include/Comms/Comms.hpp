#ifndef COMMS_H
#define COMMS_H

#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "plog/Log.h"

#include "CommsUtil.hpp"
#include "CommsLink.hpp"
#include "CommsUtilAccessor.hpp"

/*
Comms is designed to be a general communications system for many different interfaces.
In the process, I may have accidentally reimplemented Boost::Any....
Most of the functions are self documenting, but a few notable requirements are
the use of a comms_util::Hint which, in some cases, allows a CommsLink to figure
out what data type is being transferred for formatting and parsing reasons.
Additionally, as access is mutex protected, there is a function to authenticate
a CommsLink derivative to access a Comms instance mutex by passing a Comms
reference and the buffer_map reference it was given.

*/

// 1. A single mutex is used to protect all buffers - a better solution would be to have one mutex protect the changing of the links, and another mutex
// linked to each inner_map_t. That would lead to many fewer access conflicts.
// 2. Where possible, the mutex was locked and unlocked directly to minimize lock time, however for try/catch blocks, a lock_guard was used to avoid
// having to deal with all the possible exception paths.

class Comms {
public:
	static const bool CopyLocal;

	Comms() {}

	friend std::mutex* comms_util::getOpMutexFrom(Comms&, const std::string&, std::shared_ptr<comms_util::inner_map_t>);

	bool addLink(const std::string& link_id, std::shared_ptr<CommsLink> link, const bool copy_local = false);

	template<typename T>
	bool send(const std::string& link_id, const std::string& field_name, comms_util::Hint type_hint, T field_value);
	bool receive(const std::string& link_id);
	void receiveAll(void);

	template<typename T>
	const T get(const std::string& link_id, const std::string& field_name);

	template<typename T>
	bool isSetAs(const std::string& link_id, const std::string& field_name);

	bool isSet(const std::string& link_id, const std::string& field_name);
	bool hasNew(const std::string& link_id, const std::string& field_name, const std::chrono::steady_clock::time_point& previous_access);

	bool linkExists(const std::string& link_id);

private:
	std::unordered_map<std::string, std::shared_ptr<comms_util::inner_map_t> > buffer_map;
	std::unordered_map<std::string, std::shared_ptr<CommsLink>> link_map;
	std::unordered_map<std::string, bool> opts_map;

	std::mutex op_mutex;

	bool testLinkId(const std::string& link_id);
	bool testFieldInLink(const std::string& link_id, const std::string& field_name);
};

// TEMPLATE IMPLEMENTATIONS

template<typename T>
bool Comms::send(const std::string& link_id, const std::string& field_name, comms_util::Hint type_hint, T field_value) {
	if(testLinkId(link_id)) {
		auto typed_ptr = std::make_shared<comms_util::TypedDataTS<T>>(type_hint, field_value); // construct the data point
		std::shared_ptr<comms_util::DataTS> untyped_ptr = typed_ptr; // and an untyped pointer to it

		op_mutex.lock();

		if(opts_map.at(link_id) == Comms::CopyLocal) { // copy data being sent to the link's buffer if the option is set
			std::shared_ptr<comms_util::inner_map_t> inner_map = buffer_map.at(link_id);
			inner_map->erase(field_name);
			inner_map->emplace(field_name, untyped_ptr);
		}

		std::shared_ptr<CommsLink> link = link_map.at(link_id);
		op_mutex.unlock();

		link->send(field_name, untyped_ptr);
		return true;
	}
	return false;
}

template<typename T>
const T Comms::get(const std::string& link_id, const std::string& field_name) {
	if(testFieldInLink(link_id, field_name)) {
		op_mutex.lock();
		std::shared_ptr< comms_util::DataTS > base_ptr = buffer_map.at(link_id)->at(field_name);
		op_mutex.unlock();

		std::shared_ptr< comms_util::TypedDataTS<T> > typed_ptr = std::dynamic_pointer_cast< comms_util::TypedDataTS<T> >( base_ptr );
		if(typed_ptr) return typed_ptr->getContents();
	}

	LOG_WARNING << "Error in Comms::get() for '" << field_name << "' in '" << link_id << "', returning default for type.";
	return T();
}

template<typename T>
bool Comms::isSetAs(const std::string& link_id, const std::string& field_name) {
	if(testFieldInLink(link_id, field_name)) {
		op_mutex.lock();
		std::shared_ptr<comms_util::DataTS> base_ptr = buffer_map.at(link_id)->at(field_name);
		op_mutex.unlock();

		std::shared_ptr< comms_util::TypedDataTS<T> > typed_ptr = std::dynamic_pointer_cast< comms_util::TypedDataTS<T> >( base_ptr );
		if(typed_ptr) return true;
	}
	return false;
}

#endif