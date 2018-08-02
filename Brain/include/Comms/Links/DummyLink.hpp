#ifndef LOOP_LINK_H
#define LOOP_LINK_H

#include <string>
#include <memory>

#include "../CommsLink.hpp"

/*
DummyLink is an example implementation of a CommsLink and also can be stretched
into a thread-safe global store by using the Comms::CopyLocal flag when
constructed. In that form it is simply a reason for Comms to manage a buffer_map,
the data isn't sent anywhere.
*/

class DummyLink : public CommsLink {
public:
	DummyLink();
	void send(const std::string& field_name, std::shared_ptr<comms_util::DataTS> data);
	void receive();
private:

};

#endif