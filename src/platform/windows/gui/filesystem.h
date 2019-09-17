#pragma once

// Android has filesystem headers, but required library is missing
// macOS has filesystem since 10.15 - I wanna support older macOS versions
// For these systems GHC fallback is used.

#if defined(__has_include) && __has_include(<filesystem>) && !defined(ANDROID) && !defined(__APPLE__)
#include <filesystem>
namespace fs = std::filesystem;
#elif defined(__has_include) && __has_include(<experimental/filesystem>) && !defined(ANDROID) && !defined(__APPLE__)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif