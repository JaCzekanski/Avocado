#include "freecam_controller.h"
#include <fmt/core.h>
#include "config.h"
#include "input/input_manager.h"
#include "utils/screenshot.h";

namespace peripherals {
void FreecamController::update(std::string path) {
    auto inputManager = InputManager::getInstance();
    if (inputManager == nullptr) {
        return;
    }
    Screenshot* screenshot = Screenshot::getInstance();
    if (screenshot->freeCamEnabled) {
        int32_t inputX;
        int32_t inputY;
        int32_t inputZ;
        inputX = (+inputManager->getAnalog(path + "freecam_left").value - inputManager->getAnalog(path + "freecam_right").value);
        inputY = (+inputManager->getAnalog(path + "freecam_up").value - inputManager->getAnalog(path + "freecam_down").value);
        inputZ = (-inputManager->getAnalog(path + "freecam_forward").value + inputManager->getAnalog(path + "freecam_backward").value);
        int32_t rotX;
        int32_t rotY;
        int32_t rotZ;
        rotX = (-inputManager->getAnalog(path + "freecam_look_left").value + inputManager->getAnalog(path + "freecam_look_right").value);
        rotY = (+inputManager->getAnalog(path + "freecam_look_up").value - inputManager->getAnalog(path + "freecam_look_down").value);
        rotZ = (-inputManager->getAnalog(path + "freecam_look_forward").value
                + inputManager->getAnalog(path + "freecam_look_backward").value);
        screenshot->processInput(inputX, inputY, inputZ, rotX, rotY, rotZ);
    }
}
};  // namespace peripherals