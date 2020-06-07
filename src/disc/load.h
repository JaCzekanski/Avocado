#pragma once
#include <memory>
#include <string>
#include "disc.h"

namespace disc {
bool isDiscImage(const std::string& path);
std::unique_ptr<disc::Disc> load(const std::string& path);
}  // namespace disc