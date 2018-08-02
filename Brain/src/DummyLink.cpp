#include "Comms/Links/DummyLink.hpp"

#include <string>

DummyLink::DummyLink() : CommsLink() {}

using namespace comms_util;
void DummyLink::send(const std::string& field_name, std::shared_ptr<DataTS> data) {}

void DummyLink::receive() {}