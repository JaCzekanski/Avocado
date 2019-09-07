#include "abstract_device.h"

namespace peripherals {
AbstractDevice::AbstractDevice(Type type, int port) : type(type), port(port) {}

AbstractDevice::~AbstractDevice() {}

bool AbstractDevice::getAck() { return state != 0; }

void AbstractDevice::resetState() { state = 0; }
void AbstractDevice::update() {}
};  // namespace peripherals