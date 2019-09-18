#pragma once
#include "utils/json_enum.h"

enum class ControllerType {
    none,
    digital,
    analog,
    mouse,
};

JSON_ENUM(ControllerType);