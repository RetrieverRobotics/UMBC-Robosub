#include "Comms/CommsUtilAccessor.hpp"
#include "Comms/Comms.hpp"

std::mutex* comms_util::getOpMutexFrom(Comms& comms, const std::string& link_id, std::shared_ptr<comms_util::inner_map_t> asserted_map) {
	try {
		std::shared_ptr<comms_util::inner_map_t> actual_map = comms.buffer_map.at(link_id);
		if(actual_map == asserted_map) {
			return &(comms.op_mutex);
		}
	} catch(std::out_of_range& e) {
		std::cerr << "Link id: " << link_id << " does not exist." << std::endl;
	}
	return NULL;
}
