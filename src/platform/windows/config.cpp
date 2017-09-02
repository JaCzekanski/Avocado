#include "config.h"
#include "utils/file.h"

const char* CONFIG_NAME = "config.json";

// clang-format off
nlohmann::json config = {
	{"initialized", false}, 
	{"bios", ""}, 
	{"extension", ""}, 
	{"iso", ""}
};
// clang-format on

void saveConfigFile(const char* configName) { putFileContents(configName, config.dump(4)); }

void loadConfigFile(const char* configName) {
    auto file = getFileContents(configName);
    if (file.empty()) {
        saveConfigFile(configName);
        return;
    }
    nlohmann::json newconfig = nlohmann::json::parse(file);

    for (auto it = config.begin(); it != config.end(); ++it) {
        auto field = newconfig.find(it.key());

        // Add nonexisting fields
        if (field == newconfig.end()) {
            printf("Config: No field %s \n", it.key().c_str());
            newconfig.emplace(it.key(), it.value());
            continue;
        }

        // Repair these with invalid types
        if (field.value().type() != it.value().type()) {
            printf("Config: Invalid type of %s (%s), changing to %s\n", field.key().c_str(), field.value().type_name().c_str(),
                   it.value().type_name().c_str());
            newconfig[it.key()] = it.value();
        }
    }

    config = newconfig;
}
