#include "platform_tools.h"
#include <fmt/core.h>
#if defined(ANDROID)
#include <jni.h>
#include <SDL.h>
#endif

#if defined(ANDROID)
struct Activity {
    JNIEnv* env;
    jobject activity;
    jclass clazz;

    Activity() {
        env = (JNIEnv*)SDL_AndroidGetJNIEnv();
        activity = (jobject)SDL_AndroidGetActivity();
        clazz = env->GetObjectClass(activity);
    }

    ~Activity() {
        env->DeleteLocalRef(clazz);
        env->DeleteLocalRef(activity);
    }
};
bool hasExternalStoragePermission() {
    auto activity = Activity();
    auto env = activity.env;

    return env->CallBooleanMethod(activity.activity, env->GetMethodID(activity.clazz, "hasExternalStoragePermissions", "()Z"));
}
#endif

void openFileBrowser(const char* path) {
#if defined(__APPLE__)
    system(fmt::format("open \"{}\"", path).c_str());
#elif defined(__WIN32__) || defined(__WIN64__)
    system(fmt::format("explorer \"{}\"", path).c_str());
#elif defined(__LINUX__)
    system(fmt::format("xdg-open \"{}\"", path).c_str());
#endif
}