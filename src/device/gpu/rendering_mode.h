#pragma once
#include "utils/json_enum.h"

enum RenderingMode {
    software = 1 << 0,
    hardware = 1 << 1,
    mixed = software | hardware,
};
JSON_ENUM(RenderingMode);