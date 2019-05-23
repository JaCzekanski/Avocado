#include "controller.h"
#include "config.h"
#include "peripherals/analog_controller.h"
#include "peripherals/digital_controller.h"
#include "peripherals/mouse.h"
#include "peripherals/none.h"
#include "system.h"

namespace device {
namespace controller {

void Controller::handleByte(uint8_t byte) {
    rxPending = true;

    int port = ((control._reg & (1 << 13)) == 0) ? 0 : 1;

    if (deviceSelected == DeviceSelected::None) {
        if (byte == 0x01) {
            deviceSelected = DeviceSelected::Controller;
        } else if (byte == 0x81) {
            deviceSelected = DeviceSelected::MemoryCard;
        }
    }

    if (deviceSelected == DeviceSelected::Controller) {
        rxData = controller[port]->handle(byte);
        ack = controller[port]->getAck();
        if (ack) {
            irqTimer = 5;
        }
        if (controller[port]->state == 0) deviceSelected = DeviceSelected::None;
    }
    if (deviceSelected == DeviceSelected::MemoryCard) {
        rxData = card[port]->handle(byte);
        ack = card[port]->getAck();
        if (ack) {
            irqTimer = 3;
        }
        if (card[port]->state == 0) deviceSelected = DeviceSelected::None;
    }
}

Controller::Controller(System* sys) : sys(sys) {
    busToken = bus.listen<Event::Config::Controller>([&](auto) { reload(); });

    reload();

    for (auto i = 0; i < (int)card.size(); i++) {
        card[i] = std::make_unique<peripherals::MemoryCard>(i + 1);
    }
}

Controller::~Controller() { bus.unlistenAll(busToken); }
void Controller::reload() {
    auto createDevice = [](int num) -> std::unique_ptr<peripherals::AbstractDevice> {
        num += 1;
        std::string type = config["controller"][std::to_string(num)]["type"];
        if (type == ControllerType::DIGITAL) {
            return std::make_unique<peripherals::DigitalController>(num);
        } else if (type == ControllerType::ANALOG) {
            return std::make_unique<peripherals::AnalogController>(num);
        } else if (type == ControllerType::MOUSE) {
            return std::make_unique<peripherals::Mouse>(num);
        } else {
            return std::make_unique<peripherals::None>(num);
        }
    };

    for (auto i = 0; i < (int)controller.size(); i++) {
        controller[i] = createDevice(i);
    }
}

void Controller::step() {
    if (irqTimer > 0) {
        if (--irqTimer == 0) {
            irq = true;
            ack = false;
        }
    }
    if (irq) {
        sys->interrupt->trigger(interrupt::CONTROLLER);
    }
}

uint8_t Controller::read(uint32_t address) {
    if (address == 0) {  // RX
        return getData();
    }
    if (address == 4) {
        uint8_t data = (ack << 7) |        // /ACK Input Level 0 - High, 1 - Low
                                           // 6, 5, 4 - 0
                       (0 << 3) |          // Parity error
                       (1 << 2) |          // TX Ready Flag 2
                       (rxPending << 1) |  // RX FIFO Not Empty
                       (1 << 0);           // TX Ready Flag 1
        ack = false;
        return data;
    }
    if (address == 5) {
        return (irq << 1);
    }
    if (address >= 8 && address < 10) return mode._byte[address - 8];
    if (address >= 10 && address < 12) return control._byte[address - 10];
    if (address >= 14 && address < 16) return baud._byte[address - 14];
    return 0xff;
}

void Controller::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        handleByte(data);
    } else if (address >= 8 && address < 10) {
        mode._byte[address - 8] = data;
    } else if (address >= 10 && address < 12) {
        control._byte[address - 10] = data;  // mask bits 4 & 6

        if (address == 10 && (data & 0x10)) {
            irq = false;
        }
        if (address == 11) {
            if (!(control._reg & 2)) {
                // Reset periperals
                deviceSelected = DeviceSelected::None;
                for (auto& ctrl : controller) ctrl->resetState();
                for (auto& crd : card) crd->resetState();
            }
        }
    } else if (address >= 14 && address < 16) {
        baud._byte[address - 14] = data;
    }
}

void Controller::update() {
    for (auto& ctrl : controller) ctrl->update();
}
}  // namespace controller
}  // namespace device
