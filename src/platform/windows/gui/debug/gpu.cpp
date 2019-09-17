#include "gpu.h"
#include <imgui.h>

namespace gui::debug {

void GPU::registersWindow(System *sys) {
    auto &gpu = sys->gpu;

    ImGui::Begin("GPU", &showGpuWindow, ImVec2(200, 100));

    int horRes = gpu->gp1_08.getHorizontalResoulution();
    int verRes = gpu->gp1_08.getVerticalResoulution();
    bool interlaced = gpu->gp1_08.interlace;
    int mode = gpu->gp1_08.videoMode == gpu::GP1_08::VideoMode::ntsc ? 60 : 50;
    int colorDepth = gpu->gp1_08.colorDepth == gpu::GP1_08::ColorDepth::bit24 ? 24 : 15;

    ImGui::Text("Display:");
    ImGui::Text("Resolution %dx%d%c @ %dHz", horRes, verRes, interlaced ? 'i' : 'p', mode);
    ImGui::Text("Color depth: %dbit", colorDepth);
    ImGui::Text("areaStart:  %4d:%4d", gpu->displayAreaStartX, gpu->displayAreaStartY);
    ImGui::Text("rangeX:     %4d:%4d", gpu->displayRangeX1, gpu->displayRangeX2);
    ImGui::Text("rangeY:     %4d:%4d", gpu->displayRangeY1, gpu->displayRangeY2);

    ImGui::Text("");
    ImGui::Text("Drawing:");
    ImGui::Text("areaMin:    %4d:%4d", gpu->drawingArea.left, gpu->drawingArea.top);
    ImGui::Text("areaMax:    %4d:%4d", gpu->drawingArea.right, gpu->drawingArea.bottom);
    ImGui::Text("offset:     %4d:%4d", gpu->drawingOffsetX, gpu->drawingOffsetY);
    // ImGui::Text("")

    ImGui::End();
}

void GPU::logWindow(System *sys) {
    vramAreas.clear();
    if (!gpuLogEnabled) {
        return;
    }
    static std::unique_ptr<Texture> textureImage;
    static std::vector<uint8_t> textureUnpacked;
    if (!textureImage) {
        textureImage = std::make_unique<Texture>(512, 512, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, false);
        textureUnpacked.resize(512 * 512 * 4);
    }

    ImGui::Begin("GPU Log", &gpuLogEnabled, ImVec2(300, 400));

    ImGui::BeginChild("GPU Log", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    // TODO: Find last values
    gpu::GP0_E1 last_e1 = sys->gpu->gp0_e1;
    int16_t last_offset_x = sys->gpu->drawingOffsetX;
    int16_t last_offset_y = sys->gpu->drawingOffsetY;

    int renderTo = -1;
    ImGuiListClipper clipper((int)sys->gpu.get()->gpuLogList.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            auto &entry = sys->gpu.get()->gpuLogList[i];
            bool nodeOpen = ImGui::TreeNode((void *)(intptr_t)i, "%4d cmd: 0x%02x  %s", i, entry.command,
                                            std::string(magic_enum::enum_name(entry.cmd)).c_str());
            bool isHovered = ImGui::IsItemHovered();

            if (isHovered) {
                renderTo = i;
            }

            // Analyze
            if (isHovered || nodeOpen) {
                bool showArguments = true;
                float color[3];
                color[0] = (entry.args[0] & 0xff) / 255.f;
                color[1] = ((entry.args[0] >> 8) & 0xff) / 255.f;
                color[2] = ((entry.args[0] >> 16) & 0xff) / 255.f;

                uint8_t command = entry.command;
                auto arguments = entry.args;
                if (command >= 0x20 && command < 0x40) {
                    auto arg = gpu::PolygonArgs(command);
                    (void)arg;  // TODO: Parse Polygon commands
                } else if (command >= 0x40 && command < 0x60) {
                    auto arg = gpu::LineArgs(command);
                    (void)arg;  // TODO: Parse Line commands
                } else if (command >= 0x60 && command < 0x80) {
                    auto arg = gpu::RectangleArgs(command);
                    int16_t w = arg.getSize();
                    int16_t h = arg.getSize();

                    if (arg.size == 0) {
                        w = extend_sign<10>(arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff);
                        h = extend_sign<10>((arguments[(arg.isTextureMapped ? 3 : 2)] & 0xffff0000) >> 16);
                    }

                    int16_t x = extend_sign<10>(arguments[1] & 0xffff);
                    int16_t y = extend_sign<10>((arguments[1] & 0xffff0000) >> 16);

                    x += last_offset_x;
                    y += last_offset_y;

                    if (nodeOpen) {
                        std::string flags;
                        if (arg.semiTransparency) flags += "semi-transparent, ";
                        if (arg.isTextureMapped) flags += "textured, ";
                        if (!arg.isRawTexture) flags += "color-blended, ";
                        ImGui::Text("Flags: %s", flags.c_str());
                        ImGui::Text("Pos: %dx%d", x, y);
                        ImGui::Text("size: %dx%d", w, h);

                        ImGui::NewLine();
                        ImGui::Text("Color: ");
                        ImGui::SameLine();
                        ImGui::ColorEdit3("##color", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);
                    }

                    vramAreas.push_back({fmt::format("Rectangle {}", i), ImVec2(x, y), ImVec2(w, h)});

                    if (arg.isTextureMapped) {
                        int texX = arguments[2] & 0xff;
                        int texY = (arguments[2] & 0xff00) >> 8;
                        int clutX = ((arguments[2] >> 16) & 0x3f) * 16;
                        int clutY = ((arguments[2] >> 22) & 0x1ff);
                        int clutColors;
                        int textureWidth;
                        int textureBits;

                        if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit4) {
                            clutColors = 16;
                            textureWidth = w / 4;
                            textureBits = 4;
                        } else if (last_e1.texturePageColors == gpu::GP0_E1::TexturePageColors::bit8) {
                            clutColors = 256;
                            textureWidth = w / 2;
                            textureBits = 8;
                        } else {
                            clutColors = 0;
                            textureWidth = w;
                            textureBits = 16;
                        }

                        texX += last_e1.texturePageBaseX * 64;
                        texY += last_e1.texturePageBaseY * 256;

                        std::string textureInfo = fmt::format("Texture ({} bit)", textureBits);

                        if (nodeOpen) {
                            ImGui::NewLine();
                            ImGui::Text("%s: ", textureInfo.c_str());
                            ImGui::Text("Pos:  %dx%d", texX, texY);
                            ImGui::Text("CLUT: %dx%d", clutX, clutY);
                            // ImGui::SameLine();
                            // ImGui::Image()
                            // TODO: Render texture
                        }

                        vramAreas.push_back({textureInfo, ImVec2(texX, texY), ImVec2(textureWidth, h)});

                        if (clutColors != 0) {
                            vramAreas.push_back({"CLUT", ImVec2(clutX, clutY), ImVec2(clutColors, 1)});
                        }
                    }

                    showArguments = false;
                }

                if (nodeOpen) {
                    if (showArguments) {
                        // Render arguments
                        for (auto &arg : entry.args) {
                            ImGui::Text("- 0x%08x", arg);
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    if (ImGui::Button("Dump")) {
        ImGui::OpenPopup("Save dump dialog");
    }

    if (ImGui::BeginPopupModal("Save dump dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char filename[32];
        ImGui::Text("File: ");
        ImGui::SameLine();

        ImGui::PushItemWidth(140);
        if (ImGui::InputText("", filename, 31, ImGuiInputTextFlags_EnterReturnsTrue)) {
            auto gpu = sys->gpu.get();
            auto gpuLog = gpu->gpuLogList;
            nlohmann::json j;

            for (size_t i = 0; i < gpuLog.size(); i++) {
                auto e = gpuLog[i];
                j.push_back({
                    {"command", e.command},                  //
                    {"cmd", (int)e.cmd},                     //
                    {"name", magic_enum::enum_name(e.cmd)},  //
                    {"args", e.args},                        //
                });
            }

            putFileContents(fmt::format("{}.json", filename), j.dump(2));

            // Binary vram dump
            std::vector<uint8_t> vram;
            for (auto d : gpu->vram) {
                vram.push_back(d & 0xff);
                vram.push_back((d >> 8) & 0xff);
            }
            putFileContents(fmt::format("{}.bin", filename), vram);

            ImGui::CloseCurrentPopup();
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::Text("(press Enter to add)");
        ImGui::EndPopup();
    }

    ImGui::End();

    if (sys->state != System::State::run && renderTo >= 0) {
        replayCommands(sys->gpu.get(), renderTo);
    }
}

void GPU::displayWindows(System *sys) {
    if (registersWindowopen) registersWindow(sys);
    if (logWindowOpen) logWindow(sys);
}
};  // namespace gui::debug