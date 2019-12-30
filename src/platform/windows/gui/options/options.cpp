#include "options.h"
#include <fmt/core.h>
#include <imgui.h>
#include <magic_enum.hpp>
#include "config.h"
#include "device/controller/controller_type.h"
#include "platform/windows/gui/filesystem.h"
#include "platform/windows/gui/images.h"
#include "platform/windows/input/sdl_input_manager.h"
#include "renderer/opengl/opengl.h"
#include "utils/file.h"

bool showGraphicsOptionsWindow = false;
bool showBiosWindow = false;
bool showControllerSetupWindow = false;

// TODO: Move these windows to separate classes
void graphicsOptionsWindow() {
    const int MAX_LEN = 5;
    static bool initialized = false;
    static char _width[MAX_LEN + 1] = {0};
    static char _height[MAX_LEN + 1] = {0};

    const int baseWidth = 640;
    const int baseHeight = 480;

    const std::array<const char*, 5> resolutions = {{"Custom", "1x (640x480)", "2x (1280x960)", "3x (1920x1440)", "4x (2560x1920)"}};
    static int selectedResolution = 0;

    const std::array<const char*, 3> renderingModes
        = {{"Software (slow, accurate)", "Hardware (fast, pretty)", "Software + Hardware (pretty and accurate, but slow)"}};
    static int selectedRenderingMode = 0;

    auto setResolutionToEditText = [&](int w, int h) {
        snprintf(_width, MAX_LEN, "%u", w);
        snprintf(_height, MAX_LEN, "%u", h);
    };

    auto parseResolution = [&] {
        int width = 0;
        int height = 0;

        if (sscanf(_width, "%u", &width) != 1 || width < 1) return;
        if (sscanf(_height, "%u", &height) != 1 || height < 1) return;

        config["options"]["graphics"]["resolution"]["width"] = width;
        config["options"]["graphics"]["resolution"]["height"] = height;
        bus.notify(Event::Config::Graphics{});
    };

    // Load
    if (!initialized) {
        initialized = true;

        int w = config["options"]["graphics"]["resolution"]["width"];
        int h = config["options"]["graphics"]["resolution"]["height"];

        float mulW = static_cast<float>(w) / baseWidth;
        float mulH = static_cast<float>(h) / baseHeight;

        if (mulW - static_cast<int>(mulW) < 0.001f && mulH - static_cast<int>(mulH) < 0.001f
            && static_cast<int>(mulW) == static_cast<int>(mulH)) {
            selectedResolution = static_cast<int>(mulW);
        }

        selectedRenderingMode = (int)config["options"]["graphics"]["rendering_mode"].get<RenderingMode>() - 1;

        setResolutionToEditText(w, h);
    }

    ImGui::Begin("Graphics", &showGraphicsOptionsWindow, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Rendering mode");
    ImGui::SameLine();
    if (ImGui::Combo("##rendering_mode", &selectedRenderingMode, renderingModes.data(), renderingModes.size())) {
        config["options"]["graphics"]["rendering_mode"] = static_cast<RenderingMode>(selectedRenderingMode + 1);
        bus.notify(Event::Config::Graphics{});
    }

    if (selectedRenderingMode != 0) {
        ImGui::Text("Internal resolution");
        ImGui::SameLine();
        if (ImGui::Combo("##resolution_presets", &selectedResolution, resolutions.data(), resolutions.size()) && selectedResolution > 0) {
            auto w = baseWidth * selectedResolution;
            auto h = baseHeight * selectedResolution;
            setResolutionToEditText(w, h);
            parseResolution();
        }

        if (selectedResolution == 0) {
            ImGui::Text("Custom resolution: ");
            ImGui::SameLine();

            ImGui::PushItemWidth(50);
            if (ImGui::InputText("##width", _width, MAX_LEN, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal)) {
                parseResolution();
            }

            ImGui::SameLine();
            ImGui::Text("x");
            ImGui::SameLine();

            if (ImGui::InputText("##height", _height, MAX_LEN, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal)) {
                parseResolution();
            }
            ImGui::PopItemWidth();
        }
    }

    bool widescreen = config["options"]["graphics"]["widescreen"];
    if (ImGui::Checkbox("Widescreen (16/9)", &widescreen)) {
        config["options"]["graphics"]["widescreen"] = widescreen;
        if (!widescreen) {
            config["options"]["graphics"]["forceWidescreen"] = false;
        }
        bus.notify(Event::Config::Gte{});
    }

    bool vsync = config["options"]["graphics"]["vsync"];
    if (ImGui::Checkbox("VSync", &vsync)) {
        config["options"]["graphics"]["vsync"] = vsync;
        bus.notify(Event::Config::Graphics{});
    }

    ImGui::Separator();
    ImGui::Text("Hacks");

    bool forceNtsc = config["options"]["graphics"]["forceNtsc"];
    if (ImGui::Checkbox("Force NTSC", &forceNtsc)) {
        config["options"]["graphics"]["forceNtsc"] = forceNtsc;
        bus.notify(Event::Config::Graphics{});
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(
            "Force 60Hz mode for PAL games (50Hz)\n"
            "Since some PAL games are slowed down versions of NTSC equivalent,\n"
            "this option might be useful to bring back such titles to the original speed.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    if (widescreen) {
        bool forceWidescreen = config["options"]["graphics"]["forceWidescreen"];
        if (ImGui::Checkbox("Force widescreen in 3d", &forceWidescreen)) {
            config["options"]["graphics"]["forceWidescreen"] = forceWidescreen;
            bus.notify(Event::Config::Gte{});
        }
    }

    ImGui::End();
}

void biosSelectionWindow() {
    static bool biosesFound = false;
    static std::vector<std::string> bioses;
    static int selectedBios = 0;

    try {
        if (!biosesFound) {
            bioses.clear();
            auto dir = fs::directory_iterator("data/bios");
            for (auto& e : dir) {
                if (!fs::is_regular_file(e)) continue;

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
    } catch (fs::filesystem_error& err) {
        fmt::print("{}", err.what());
    }

    ImGui::Begin("BIOS", &showBiosWindow, ImGuiWindowFlags_AlwaysAutoResize);
    if (bioses.empty()) {
        ImGui::Text(
            "BIOS directory is empty.\n"
            "You need one of BIOS files (eg. SCPH1001.bin) placed in data/bios directory.\n"
            ".bin and .rom extensions are recognised.");
    } else {
        ImGui::ListBox(
            "", &selectedBios,
            [](void* data, int idx, const char** out_text) {
                const std::vector<std::string>* v = (std::vector<std::string>*)data;
                *out_text = v->at(idx).c_str();
                return true;
            },
            (void*)&bioses, (int)bioses.size());

        if (ImGui::Button("Select", ImVec2(-1, 0)) && selectedBios < (int)bioses.size()) {
            std::string biosPath = bioses[selectedBios];
// SDL treats relative paths (not starting with /) as relative to app internal storage path
// Storing path as absolute is needed for file to be found
#ifdef ANDROID
            biosPath = fs::absolute(biosPath).string();
#endif
            config["bios"] = biosPath;

            biosesFound = false;
            showBiosWindow = false;
            bus.notify(Event::System::HardReset{});
        }
    }
    ImGui::End();
}

void drawImage(const std::optional<Image> image, float w = 0.f, float h = 0.f) {
    if (auto img = image) {
        ImVec2 size;

        if (w != 0.f && h != 0.f) {
            size = ImVec2(w, h);
        } else if (w != 0.f) {
            float aspect = static_cast<float>(img->h) / static_cast<float>(img->w);
            size = ImVec2(w, w * aspect);
        } else if (h != 0.f) {
            float aspect = static_cast<float>(img->w) / static_cast<float>(img->h);
            size = ImVec2(h * aspect, h);
        } else {
            size = ImVec2((float)img->w, (float)img->h);
        }
        ImGui::Image((ImTextureID)(uintptr_t)img->id, size);
    }
}

void button(int controller, const std::string& button, const char* tooltip = nullptr) {
    auto inputManager = static_cast<SdlInputManager*>(InputManager::getInstance());
    static std::string currentButton = "";
    if (controller < 1 || controller > 4) return;
    std::string ctrl = std::to_string(controller);

    if (button == currentButton && inputManager->lastPressedKey.type != Key::Type::None) {
        config["controller"][ctrl]["keys"][button] = inputManager->lastPressedKey.to_string();
        inputManager->lastPressedKey = Key();
    }

    std::string key = config["controller"][ctrl]["keys"][button];

    const float iconSize = 20.f;
    drawImage(getImage(button, "data/assets/buttons"), iconSize);
    if (ImGui::IsItemHovered() && tooltip != nullptr) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }
    ImGui::SameLine();

    std::optional<Image> image = {};
    std::string deviceIndex = "";

    if (key.rfind("keyboard") == 0) {
        image = getImage("ic_keyboard");
    } else if (key.rfind("mouse") == 0) {
        image = getImage("ic_mouse");
    } else if (key.rfind("controller") == 0) {
        image = getImage("ic_controller");
        deviceIndex = key.substr(10, key.find("|") - 10);
    }

    if (auto pos = key.find("|"); pos != std::string::npos) {
        key = key.substr(pos + 1);
    }

    if (ImGui::Button(fmt::format("{}##{}", key, button).c_str(), ImVec2(-iconSize * 3, 0.f))) {
        currentButton = button;
        inputManager->waitingForKeyPress = true;
        ImGui::OpenPopup("Waiting for key...");

        // TODO: Detect duplicates
    }

    if (image) {
        ImGui::SameLine();
        drawImage(image, iconSize);
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(deviceIndex.c_str());
}

void controllerSetupWindow() {
    const int controllerCount = 2;
    static int selectedController = 1;
    static auto comboString = fmt::format("Controller {}", selectedController);

    const auto controllerTypes = magic_enum::enum_entries<ControllerType>();

    const std::array<const char*, 7> defaults = {{
        "Defaults ...", "Clear", "Keyboard (WADX)", "Keyboard (Numpad)", "Mouse", "Controller 1",
        "Controller 2",  // TODO: Add autodetection for game controllers
    }};

    ImGui::SetNextWindowSize(ImVec2(500.f, 320.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controller", &showControllerSetupWindow, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushItemWidth(-1);
    if (ImGui::BeginCombo("##combo_controller", comboString.c_str())) {
        for (int i = 1; i <= controllerCount; i++) {
            bool isSelected = i == selectedController;
            auto label = fmt::format("Controller {}", i);
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
    ImGui::PopItemWidth();

    auto currentType = config["controller"][std::to_string(selectedController)]["type"];

    const float leftGroupWidth = 256.f;
    ImGui::BeginGroup();
    ImGui::BeginChild("##controller_image", ImVec2(leftGroupWidth, -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Text("Type");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::BeginCombo("##type", std::string(currentType).c_str())) {
        for (auto& type : controllerTypes) {
            if (ImGui::Selectable(std::string(type.second).c_str())) {
                config["controller"][std::to_string(selectedController)]["type"] = type.first;
                bus.notify(Event::Config::Controller{});
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (currentType != ControllerType::none) {
        drawImage(getImage(currentType), leftGroupWidth);
    }
    ImGui::EndChild();

    if (currentType != ControllerType::none) {
        ImGui::PushItemWidth(leftGroupWidth);
        if (int pos = 0; ImGui::Combo("##defaults", &pos, defaults.data(), (int)defaults.size())) {
            auto& keysConfig = config["controller"][std::to_string(selectedController)]["keys"];
            switch (pos) {
                case 1: keysConfig = DefaultKeyBindings::none(); break;
                case 2: keysConfig = DefaultKeyBindings::keyboard_wadx(); break;
                case 3: keysConfig = DefaultKeyBindings::keyboard_numpad(); break;
                case 4: keysConfig = DefaultKeyBindings::mouse(); break;
                case 5: keysConfig = DefaultKeyBindings::controller(1); break;
                case 6: keysConfig = DefaultKeyBindings::controller(2); break;
            }
        }
        ImGui::PopItemWidth();
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginChild("Buttons", ImVec2(0.f, 0.f));
    if (currentType == ControllerType::mouse) {
        button(selectedController, "l_up", "Move Up");
        button(selectedController, "l_right", "Move Right");
        button(selectedController, "l_down", "Move Down");
        button(selectedController, "l_left", "Move Left");
        button(selectedController, "l1", "Left Button");
        button(selectedController, "r1", "Right Button");
    }
    if (currentType == ControllerType::digital || currentType == ControllerType::analog) {
        button(selectedController, "dpad_up", "D-Pad Up");
        button(selectedController, "dpad_down", "D-Pad Down");
        button(selectedController, "dpad_left", "D-Pad Left");
        button(selectedController, "dpad_right", "D-Pad Right");

        button(selectedController, "triangle", "Triange");
        button(selectedController, "cross", "Cross");
        button(selectedController, "square", "Square");
        button(selectedController, "circle", "Circle");

        button(selectedController, "l1", "L1");
        button(selectedController, "r1", "R1");
        button(selectedController, "l2", "L2");
        button(selectedController, "r2", "R2");

        button(selectedController, "select", "Select");
        button(selectedController, "start", "Start");

        if (currentType == ControllerType::analog) {
            button(selectedController, "analog", "Analog mode toggle");
            button(selectedController, "l3", "Left Stick Press");
            button(selectedController, "l_up", "Left Stick Up");
            button(selectedController, "l_right", "Left Stick Right");
            button(selectedController, "l_down", "Left Stick Down");
            button(selectedController, "l_left", "Left Stick Left");
            button(selectedController, "r3", "Right Stick Press");
            button(selectedController, "r_up", "Right Stick Up");
            button(selectedController, "r_right", "Right Stick Right");
            button(selectedController, "r_down", "Right Stick Down");
            button(selectedController, "r_left", "Right Stick Left");
        }
    }

    auto inputManager = static_cast<SdlInputManager*>(InputManager::getInstance());
    if (ImGui::BeginPopupModal("Waiting for key...", &inputManager->waitingForKeyPress, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Press any key (ESC to cancel).");
        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::End();
}
