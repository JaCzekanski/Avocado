#include "config_observer.h"

void ConfigObserver::registerCallback(const Event name, Callback callback) {
    observers.emplace(name, callback);
}
void ConfigObserver::unregisterCallback(const Event name) {
    observers.erase(name);
}
void ConfigObserver::notify(const Event name) {
    auto observer = observers.find(name);
    if (observer != observers.end()) {
        observer->second();
    }
}