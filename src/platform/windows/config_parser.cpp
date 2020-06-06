#include "config_parser.h"
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include "config.h"
#include "device/controller/controller_type.h"
#include "device/gpu/rendering_mode.h"
#include "utils/file.h"
#include "utils/json_enum.h"
#include "config.h"

// For now .json format is used for config serialization.
// Might switch to .toml some day

const char* CONFIG_NAME = "config.json";
std::string configPath() { return avocado::PATH_USER + CONFIG_NAME; }

JSON_ENUM(ControllerType);
JSON_ENUM(RenderingMode);

void saveConfigFile() {
    nlohmann::json json;
    json["bios"] = config.bios;
    json["extension"] = config.extension;
    json["iso"] = config.iso;

    for (int i = 0; i < 2; i++) {
        json["controller"][std::to_string(i + 1)] = {
            {"type", config.controller[i].type},
            {"keys", config.controller[i].keys},
        };
    }

    auto g = config.options.graphics;
    json["options"]["graphics"] = {
        {"renderingMode", g.renderingMode},
        {"widescreen", g.widescreen},
        {"forceWidescreen", g.forceWidescreen},
        {
            "resolution",
            {
                {"width", g.resolution.width},
                {"height", g.resolution.height},
            },
        },
        {"vsync", g.vsync},
        {"forceNtsc", g.forceNtsc},
    };

    json["options"]["sound"] = {
        {"enabled", config.options.sound.enabled},
    };

    json["options"]["emulator"] = {
        {"preserveState", config.options.emulator.preserveState},
        {"timeTravel", config.options.emulator.timeTravel},
    };

    auto l = config.debug.log;
    json["debug"]["log"] = {
        {"bios", l.bios},              //
        {"cdrom", l.cdrom},            //
        {"controller", l.controller},  //
        {"dma", l.dma},                //
        {"gpu", l.gpu},                //
        {"gte", l.gte},                //
        {"mdec", l.mdec},              //
        {"memoryCard", l.memoryCard},  //
        {"spu", l.spu},                //
        {"system", l.system},          //
    };

    for (int i = 0; i < 2; i++) {
        json["memoryCard"][std::to_string(i + 1)] = config.memoryCard[i].path;
    }

    json["gui"] = {
        {"lastPath", config.gui.lastPath},
    };

    putFileContents(configPath(), json.dump(4));
}

void loadConfigFile() {
    auto file = getFileContents(configPath());
    if (file.empty()) {
        saveConfigFile();
        return;
    }
    nlohmann::json json = nlohmann::json::parse(file.begin(), file.end());

    try {
        config.bios = json["bios"];
        config.extension = json["extension"];
        config.iso = json["iso"];

        for (int i = 0; i < 2; i++) {
            auto ctrl = json["controller"][std::to_string(i + 1)];
            if (ctrl.is_null()) continue;

            config.controller[i].type = ctrl["type"];
            config.controller[i].keys = ctrl["keys"].get<KeyBindings>();
        }

        if (auto g = json["options"]["graphics"]; !g.is_null()) {
            config.options.graphics.renderingMode = g["renderingMode"];
            config.options.graphics.widescreen = g["widescreen"];
            config.options.graphics.forceWidescreen = g["forceWidescreen"];
            config.options.graphics.resolution.width = g["resolution"]["width"];
            config.options.graphics.resolution.height = g["resolution"]["height"];
            config.options.graphics.vsync = g["vsync"];
            config.options.graphics.forceNtsc = g["forceNtsc"];
        }

        if (auto s = json["options"]["sound"]; !s.is_null()) {
            config.options.sound.enabled = s["enabled"];
        }

        if (auto e = json["options"]["emulator"]; !e.is_null()) {
            config.options.emulator.preserveState = e["preserveState"];
            config.options.emulator.timeTravel = e["timeTravel"];
        }

        if (auto l = json["debug"]["log"]; !l.is_null()) {
            config.debug.log.bios = l["bios"];
            config.debug.log.cdrom = l["cdrom"];
            config.debug.log.controller = l["controller"];
            config.debug.log.dma = l["dma"];
            config.debug.log.gpu = l["gpu"];
            config.debug.log.gte = l["gte"];
            config.debug.log.mdec = l["mdec"];
            config.debug.log.memoryCard = l["memoryCard"];
            config.debug.log.spu = l["spu"];
            config.debug.log.system = l["system"];
        }

        for (int i = 0; i < 2; i++) {
            auto card = json["memoryCard"][std::to_string(i + 1)];
            if (card.is_null()) continue;

            config.memoryCard[i].path = card;
        }

        if (auto g = json["gui"]; !g.is_null()) {
            config.gui.lastPath = g["lastPath"];
        }

    } catch (nlohmann::json::exception& e) {
        fmt::print("[ERROR] Cannot load {} - {}\n", configPath(), e.what());
    }
}