#pragma once
#include <chrono>
#include <string>
#include <vector>

namespace gui {
class Toasts {
    struct Toast {
        std::string msg;
        std::chrono::steady_clock::time_point expire;
    };

    int busToken;
    std::vector<Toast> toasts;

    std::string getToasts();

   public:
    Toasts();
    ~Toasts();
    void display();
};
}  // namespace gui