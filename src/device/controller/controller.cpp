#include <fmt/core.h>
#include "controller.h"
#include "config.h"
#include "peripherals/analog_controller.h"
#include "peripherals/digital_controller.h"
#include "peripherals/mouse.h"
#include "peripherals/none.h"
#include "system.h"

namespace device {
namespace controller {

Controller::Controller(System* sys) : sys(sys) {
    verbose = config.debug.log.controller;
    busToken = bus.listen<Event::Config::Controller>([&](auto) { reload(); });

    reload();

    for (auto i = 0; i < (int)card.size(); i++) {
        card[i] = std::make_unique<peripherals::MemoryCard>(i + 1);
    }

    status.txStarted = true;
    status.txFinished = true;
}

void Controller::handleByte(uint8_t byte) {
    if (!control.select) return;
    status.txStarted = true;
    status.txFinished = false;
    if (deviceSelected == DeviceSelected::None) {
        if (byte == 0x01) {
            deviceSelected = DeviceSelected::Controller;
        } else if (byte == 0x81) {
            deviceSelected = DeviceSelected::MemoryCard;
        }
    }

    if (deviceSelected == DeviceSelected::Controller) {
        postByte(controller[control.port]->handle(byte));
        if (controller[control.port]->getAck()) {
            postAck();
        } else {
            deviceSelected = DeviceSelected::None;
        }
    }
    if (deviceSelected == DeviceSelected::MemoryCard) {
        postByte(card[control.port]->handle(byte));
        if (card[control.port]->getAck()) {
            postAck();
        } else {
            deviceSelected = DeviceSelected::None;
        }
    }
}

Controller::~Controller() { bus.unlistenAll(busToken); }
void Controller::reload() {
    auto createDevice = [](int num) -> std::unique_ptr<peripherals::AbstractDevice> {
        num += 1;
        ControllerType type = config.controller[num - 1].type;
        if (type == ControllerType::digital) {
            return std::make_unique<peripherals::DigitalController>(num);
        } else if (type == ControllerType::analog) {
            return std::make_unique<peripherals::AnalogController>(num);
        } else if (type == ControllerType::mouse) {
            return std::make_unique<peripherals::Mouse>(num);
        } else {
            return std::make_unique<peripherals::None>(num);
        }
    };

    for (auto i = 0; i < (int)controller.size(); i++) {
        controller[i] = createDevice(i);
    }
}

void Controller::step(int cpuCycles) {
    if (rxDelay > 0) {
        rxDelay -= cpuCycles;
        if (rxDelay <= 0) {
            status.txFinished = true;
            status.rxPending = true;

            if (control.txInterruptEnable) {
                sys->interrupt->trigger(interrupt::CONTROLLER);
            }
            // TODO: Rx interrupt
        }
    } else if (ackDelay > 0) {
        ackDelay -= cpuCycles;
        if (ackDelay <= 0) {
            status.ack = true;
            status.irq = true;

            if (control.ackInterruptEnable) {
                if (verbose >= 2) fmt::print("[CTRL] ACK IRQ\n");
                sys->interrupt->trigger(interrupt::CONTROLLER);
            }
        }
    } else if (ackDuration > 0) {
        ackDuration -= cpuCycles;
        if (ackDuration <= 0) {
            status.ack = false;
        }
    }
}

uint8_t Controller::read(uint32_t address) {
    auto _read = [this](uint32_t address) -> uint8_t {
        if (address == 0) {  // RX
            return getData();
        }
        // (ack << 7) |        // /ACK Input Level 0 - High, 1 - Low
        // 6, 5, 4 - 0
        // (0 << 3) |          // Parity error
        // (1 << 2) |          // TX Ready Flag 2
        // (rxPending << 1) |  // RX FIFO Not Empty
        // (1 << 0);           // TX Ready Flag 1
        if (address >= 4 && address < 8) return status._byte[address - 4];
        if (address >= 8 && address < 10) return mode._byte[address - 8];
        if (address >= 10 && address < 12) return control._byte[address - 10];
        if (address >= 14 && address < 16) return baud._byte[address - 14];
        return 0xff;
    };
    uint8_t data = _read(address);
    if (verbose >= 2) {
        if (address == 0)
            fmt::print("[CTRL] R RX: 0x{:02x}\n", data);
        else if (address == 5)
            fmt::print("[CTRL] R STATUS: 0x{:04x}\n", status._reg);
        else if (address == 7)
            fmt::print("[CTRL] R STATUS: 0x{:04x}\n", status._reg);
        else if (address == 9)
            fmt::print("[CTRL] R MODE: 0x{:04x}\n", mode._reg);
        else if (address == 11)
            fmt::print("[CTRL] R CONTROL:   0x{:04x}\n", control._reg);
        else if (address == 15)
            fmt::print("[CTRL] R BAUD: 0x{:04x}\n", baud._reg);
    }
    return data;
}

void Controller::write(uint32_t address, uint8_t data) {
    if (address == 0) {
        if (verbose >= 2) fmt::print("[CTRL] W TX: 0x{:02x}\n", data);
        handleByte(data);
    } else if (address >= 8 && address < 10) {
        mode._byte[address - 8] = data;
        if (verbose >= 2 && address == 9) fmt::print("[CTRL] W MODE: 0x{:04x}\n", mode._reg);
    } else if (address >= 10 && address < 12) {
        control._byte[address - 10] = data;
        if (verbose >= 2 && address == 10) fmt::print("[CTRL] W CONTROL_L: 0x  {:02x}\n", data);
        if (verbose >= 2 && address == 11) fmt::print("[CTRL] W CONTROL_H: 0x{:02x}\n", data);

        if (!control.select) {
            deviceSelected = DeviceSelected::None;
            for (auto& ctrl : controller) ctrl->resetState();
            for (auto& crd : card) crd->resetState();
        }

        if (control.acknowledge) {
            control.acknowledge = false;

            status.rxParityError = false;
            if (ackDuration > 0) {
                status.irq = false;
            }
        }

        if (control.reset) {
            control.reset = false;
            status.txStarted = true;
            status.rxPending = false;
            status.txFinished = true;
            fmt::print("[WARNING] [CTRL] control.reset, should not be used...\n");
        }

    } else if (address >= 14 && address < 16) {
        baud._byte[address - 14] = data;
        if (verbose >= 2 && address == 15) fmt::print("[CTRL] W BAUD: 0x{:04x}\n", baud._reg);
    }
}

void Controller::update() {
    for (auto& ctrl : controller) ctrl->update();
}

uint8_t Controller::getData() {
    uint8_t ret = rxData;
    rxData = 0xff;
    status.rxPending = false;
    return ret;
}

void Controller::postByte(uint8_t data) {
    // Controllers run at 250kHz clock
    // To send 1 bit it takes 1 / 250kHz = 4us
    // To send 8 bits it takes 4us * 8 = 32us

    // Clock is not hardcoded here - baud rate and prescaler are used instead
    int clockFrequency = timing::CPU_CLOCK / std::max((baud._reg * mode.getPrescaler()) & 0xfffffe, 1);
    int oneBitPeriod = 1'000'000 / clockFrequency;

    rxData = data;
    rxDelay = timing::usToCpuCycles(oneBitPeriod * mode.getBits());
    status.rxPending = false;
}

void Controller::postAck(int delayUs, int durationUs) {
    status.ack = false;
    ackDelay = timing::usToCpuCycles(delayUs);
    ackDuration = timing::usToCpuCycles(durationUs);
}

}  // namespace controller
}  // namespace device
