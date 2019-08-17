#include "tools.h"

namespace gui::tools {
void Columns(const std::vector<std::string>& list, size_t col_num) {
    std::vector<float> columnsWidth(col_num);
    size_t n = 0;

    auto column = [&](const std::string& str) {
        ImVec2 size = ImGui::CalcTextSize(str.c_str());
        size.x += 8.f;
        if (size.x > columnsWidth[n]) columnsWidth[n] = size.x;
        ImGui::TextUnformatted(str.c_str());
        ImGui::NextColumn();
        if (++n >= columnsWidth.size()) n = 0;
    };

    ImGui::Columns(col_num, nullptr, true);
    for (size_t c = 0; c < col_num; c++) {
        ImGui::SetColumnWidth(c, columnsWidth[c]);
    }

    for (auto item : list) {
        column(item);
    }

    ImGui::Columns(1);
}
};  // namespace gui::tools