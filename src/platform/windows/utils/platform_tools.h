#pragma once

#if defined(ANDROID)
bool hasExternalStoragePermission();
#endif

void openFileBrowser(const char* path);