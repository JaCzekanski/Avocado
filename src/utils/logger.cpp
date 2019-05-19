#include "logger.h"
#include <array>
#include <cstdarg>
#include <cstdio>
#include <regex>

namespace logger {
struct Color {
    const char* emoji;
    const char* ansi;
};
const char* COLOR_DEFAULT = "\u001b[37m";
const char* FONT_DEFAULT = "\u001b[0m";

const std::array<Color, 12> colors = {{
    {"⚫️", "\u001b[30m"},
    {"🖤", "\u001b[30m"},
    {"🔴", "\u001b[31m"},
    {"❤️", "\u001b[31m"},
    {"💚", "\u001b[32m"},
    {"💛", "\u001b[33m"},
    {"🔵", "\u001b[34m"},
    {"💙", "\u001b[34m"},
    {"💜", "\u001b[35m"},
    // No cyan
    {"⚪️", COLOR_DEFAULT},
    {"🅱️", "\u001b[1m"},  // Bold
    {"❌", FONT_DEFAULT}        // Normal
}};

void printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

#ifdef _WIN32
    puts(buffer);
#else
    for (int i = 0; i < strlen(buffer); i++) {
        for (int c = 0; c < colors.size(); c++) {
            if (strncmp(buffer + i, colors[c].emoji, strlen(colors[c].emoji)) == 0) {
                fputs(colors[c].ansi, stdout);

                i += strlen(colors[c].emoji);
                break;
            }
        }

        putchar(buffer[i]);
    }

    fputs(COLOR_DEFAULT, stdout);
    fputs(FONT_DEFAULT, stdout);
#endif
}

}  // namespace log