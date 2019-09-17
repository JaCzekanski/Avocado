#pragma once
#include <memory>
#include <string>
#include "disc.h"

namespace disc {
std::unique_ptr<disc::Disc> load(const std::string& path);
}