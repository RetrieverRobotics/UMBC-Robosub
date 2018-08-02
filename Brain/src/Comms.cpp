
#include "Comms/Comms.hpp"

const bool Comms::CopyLocal = true;

bool Comms::addLink(const std::string& link_id, std::shared_ptr<CommsLink> link, const bool copy_local) {
	std::lock_guard<std::mutex> lock(op_mutex);

	try {
		link_map.at(link_id);
		std::cerr << "Link id: " << link_id << " is already in use." << std::endl;
		return false; // link_id in use
	} catch(std::out_of_range& e) {
		// create hashmap under link_id
		buffer_map.emplace(link_id, std::make_shared<comms_util::inner_map_t>());
		// add entry to link_map
		link_map.emplace(link_id, link);

		link->init(this, link_id, buffer_map.at(link_id));

		opts_map.emplace(link_id, copy_local);

		return true;
	}	
}

bool Comms::receive(const std::string& link_id) {
	try {
		std::unique_lock<std::mutex> ulock(op_mutex);

		std::shared_ptr<CommsLink> link = link_map.at(link_id);
		ulock.unlock();

		link->receive();

		return true;
	} catch(std::out_of_range& e) { return false; }
}

void Comms::receiveAll(void) {
	for(auto it = link_map.begin(); it != link_map.end(); ++it) {
		op_mutex.lock();
		std::shared_ptr<CommsLink> link = it->second;
		op_mutex.unlock();

		link->receive();
	}
}

bool Comms::isSet(const std::string& link_id, const std::string& field_name) {
	try {
		std::unique_lock<std::mutex> ulock(op_mutex);

		buffer_map.at(link_id)->at(field_name);
		ulock.unlock();

		return true;
	} catch(std::out_of_range& e) { return false; }
}

bool Comms::hasNew(const std::string& link_id, const std::string& field_name, const std::chrono::steady_clock::time_point& previous_access) {
	if(testFieldInLink(link_id, field_name)) {
		op_mutex.lock();
		std::shared_ptr<comms_util::DataTS> base_ptr = buffer_map.at(link_id)->at(field_name);
		op_mutex.unlock();

		// verify the pointer just in case, then check if the current data is more recent that the provided time_point
		if(base_ptr && base_ptr->getTimePoint() > previous_access) return true;
	}
	return false;
}

bool Comms::testLinkId(const std::string& link_id) {
	try {
		std::unique_lock<std::mutex> ulock(op_mutex);
		buffer_map.at(link_id);
		ulock.unlock();

		return true;
	} catch(std::out_of_range& e) {
		//std::cerr << "Link with id '" << link_id << "'' does not exist." << std::endl;
	}
	return false;
}

bool Comms::testFieldInLink(const std::string& link_id, const std::string& field_name) {
	try {
		std::lock_guard<std::mutex> lock(op_mutex); // the one time a lock_guard is more applicable than a unique_lock :P

		std::shared_ptr<comms_util::inner_map_t> inner_map = buffer_map.at(link_id);
		try {
			inner_map->at(field_name);
			return true;
		} catch(std::out_of_range& e) {
			//std::cerr << "Field named '" << field_name << "' does not exist in link with id '" << link_id << "'." << std::endl;
		}
	} catch(std::out_of_range& e) {
		//std::cerr << "Link with id '" << link_id << "'' does not exist." << std::endl;
	}
	return false;
}

bool Comms::linkExists(const std::string& link_id) {
	return testLinkId(link_id);
}

