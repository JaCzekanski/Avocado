#include "tools.h"

namespace gui::tools {
void Columns(const std::vector<std::string>& list, int col_num) {
    std::vector<float> columnsWidth(col_num);
    int n = 0;

    auto column = [&](const std::string& str) {
        ImVec2 size = ImGui::CalcTextSize(str.c_str());
        size.x += 8.f;
        if (size.x > columnsWidth[n]) columnsWidth[n] = size.x;
        ImGui::TextUnformatted(str.c_str());
        ImGui::NextColumn();
        if (++n >= columnsWidth.size()) n = 0;
    };

    ImGui::Columns(col_num, nullptr, true);
    for (auto item : list) {
        column(item);
    }

    for (int c = 0; c < col_num; c++) {
        ImGui::SetColumnWidth(c, columnsWidth[c]);
    }
    ImGui::Columns(1);
}
};  // namespace gui::tools