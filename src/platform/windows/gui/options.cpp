#include "options.h"
#include <imgui.h>
#include "config.h"
#include "gui.h"
#include "renderer/opengl/opengl.h"
#include "utils/file.h"
#include "utils/string.h"

#ifdef _WIN32
#include <filesystem>
using namespace std::experimental::filesystem::v1;
#endif

bool showGraphicsOptionsWindow = false;
bool showBiosWindow = false;
bool showControllerSetupWindow = false;

void graphicsOptionsWindow() {
    bool filtering = config["options"]["graphics"]["filtering"];
    bool widescreen = config["options"]["graphics"]["widescreen"];
    ImGui::Begin("Graphics", &showGraphicsOptionsWindow, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Checkbox("Filtering", &filtering)) {
        config["options"]["graphics"]["filtering"] = filtering;
    }
    if (ImGui::Checkbox("Widescreen (16/9)", &widescreen)) {
        config["options"]["graphics"]["widescreen"] = widescreen;
    }

    ImGui::End();
}

void biosSelectionWindow() {
    static bool biosesFound = false;
    static std::vector<std::string> bioses;
    static int selectedBios = 0;

#ifdef _WIN32
    if (!biosesFound) {
        bioses.clear();
        auto dir = directory_iterator("data/bios");
        for (auto& e : dir) {
            if (!is_regular_file(e)) continue;

            auto path = e.path().string();
            auto ext = getExtension(path);
            std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

            if (ext == "bin" || ext == "rom") {
                bioses.push_back(path);
            }
        }
        biosesFound = true;

        int i = 0;
        for (auto it = bioses.begin(); it != bioses.end(); ++it, ++i) {
            if (*it == config["bios"]) {
                selectedBios = i;
                break;
            }
        }
    }
#endif

    ImGui::Begin("BIOS", &showBiosWindow, ImGuiWindowFlags_AlwaysAutoResize);
    if (bioses.empty()) {
#ifdef _WIN32
        ImGui::Text(
            "BIOS directory is empty.\n"
            "You need one of BIOS files (eg. SCPH1001.bin) placed in data/bios directory.\n"
            ".bin and .rom extensions are recognised.");
#else
        ImGui::Text(
            "No filesystem support on macOS, sorry :(\n"
            "edit bios in config.json after closing this program");
#endif
    } else {
        ImGui::PushItemWidth(300.f);
        ImGui::ListBox("", &selectedBios,
                       [](void* data, int idx, const char** out_text) {
                           const std::vector<std::string>* v = (std::vector<std::string>*)data;
                           *out_text = v->at(idx).c_str();
                           return true;
                       },
                       (void*)&bioses, (int)bioses.size());
        ImGui::PopItemWidth();

        if (ImGui::Button("Select", ImVec2(-1, 0)) && selectedBios < (int)bioses.size()) {
            config["bios"] = bioses[selectedBios];

            biosesFound = false;
            showBiosWindow = false;
            doHardReset = true;
        }
    }
    ImGui::End();
}

void button(int controller, std::string button) {
    static std::string currentButton = "";
    if (controller < 1 || controller > 4) return;
    std::string ctrl = std::to_string(controller);

    if (button == currentButton && lastPressedKey.type != Key::Type::None) {
        config["controller"][ctrl]["keys"][button] = lastPressedKey.to_string();
        lastPressedKey = Key();
    }

    std::string key = config["controller"][ctrl]["keys"][button];

    ImGui::TextUnformatted(button.c_str());
    ImGui::NextColumn();
    if (ImGui::Button(key.c_str(), ImVec2(-1.f, 0.f))) {
        currentButton = button;
        waitingForKeyPress = true;
        ImGui::OpenPopup("Waiting for key...");
    }
    ImGui::NextColumn();
}

void controllerSetupWindow() {
    const int controllerCount = 2;
    static int selectedController = 1;
    static std::string comboString = string_format("Controller %d", selectedController);

    const std::array<const char*, 4> types
        = {{ControllerType::NONE.c_str(), ControllerType::DIGITAL.c_str(), ControllerType::ANALOG.c_str(), ControllerType::MOUSE.c_str()}};

    const auto find = [&](std::string selectedType) -> int {
        for (size_t i = 0; i < types.size(); i++) {
            std::string type = types[i];
            if (type == selectedType) return i;
        }
        return 0;
    };

    if (ImGui::BeginPopupModal("Waiting for key...", &waitingForKeyPress, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Press any key (ESC to cancel).");
        ImGui::EndPopup();
    }

    ImGui::Begin("Controller", &showControllerSetupWindow);

    if (ImGui::BeginCombo("##combo_controller", comboString.c_str())) {
        for (int i = 1; i <= controllerCount; i++) {
            bool isSelected = i == selectedController;
            std::string label = string_format("Controller %d", i);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedController = i;
                comboString = label;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::BeginGroup();
    ImGui::Text("Image here");
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Type");
    ImGui::NextColumn();

    int currentType = find(config["controller"][std::to_string(selectedController)]["type"]);
    if (ImGui::Combo("##type", &currentType, types.data(), types.size())) {
        config["controller"][std::to_string(selectedController)]["type"] = types[currentType];
        configObserver.notify(Event::Controller);
    }
    ImGui::NextColumn();

    if (types[currentType] == ControllerType::DIGITAL || types[currentType] == ControllerType::ANALOG) {
        button(selectedController, "up");
        button(selectedController, "down");
        button(selectedController, "left");
        button(selectedController, "right");

        button(selectedController, "triangle");
        button(selectedController, "cross");
        button(selectedController, "square");
        button(selectedController, "circle");

        button(selectedController, "l1");
        button(selectedController, "r1");
        button(selectedController, "l2");
        button(selectedController, "r2");

        button(selectedController, "select");
        button(selectedController, "start");

        ImGui::Columns(1);

        if (types[currentType] == ControllerType::ANALOG) {
            ImGui::Text("Note: use game controller with analog sticks");
        }

        if (ImGui::Button("Restore defaults")) {
            config["controller"][std::to_string(selectedController)]["keys"]
                = defaultConfig["controller"][std::to_string(selectedController)]["keys"];
        }
    }

    ImGui::EndGroup();
    ImGui::End();
}
