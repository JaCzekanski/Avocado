#include "config.h"
#include "utils/file.h"

const char* CONFIG_NAME = "config.json";

nlohmann::json config = {{"initialized", false}, {"bios", ""}, {"extension", ""}, {"iso", ""}};

void saveConfigFile(const char* configName) { putFileContents(configName, config.dump(4)); }

void loadConfigFile(const char* configName) {
    auto file = getFileContents(configName);
    if (file.empty()) {
        saveConfigFile(configName);
        return;
    }
    config = nlohmann::json::parse(file);
}
