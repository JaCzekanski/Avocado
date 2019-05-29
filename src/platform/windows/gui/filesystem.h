#pragma once

#if defined(__has_include) && __has_include(<filesystem>) && !defined(ANDROID)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__has_include) && __has_include(<experimental/filesystem>) && !defined(ANDROID)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif