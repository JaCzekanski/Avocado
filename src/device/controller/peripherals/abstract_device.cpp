#include "abstract_device.h"
#include <cstdio>

namespace peripherals {
AbstractDevice::AbstractDevice(Type type, int port) : type(type), port(port) {}

bool AbstractDevice::getAck() { return state != 0; }

void AbstractDevice::resetState() { state = 0; }
void AbstractDevice::update() {}
};  // namespace peripherals