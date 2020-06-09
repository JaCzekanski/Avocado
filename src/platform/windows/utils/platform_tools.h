#pragma once
#include <string>

#if defined(ANDROID)
bool hasExternalStoragePermission();
#endif

void openFileBrowser(const std::string& path);