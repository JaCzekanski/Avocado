#include <string>

namespace gui::help {
class SysInfo {
    void sysinfoWindow();

   public:
    bool sysinfoWindowOpen = false;

    void displayWindows();
};

enum CPUVendor {
    VENDOR_INTEL = 0,
    VENDOR_AMD = 1,
};

struct CPUInfo {
    CPUVendor vendor;

    // Misc
    char cpu_string[0x21];
    char brand_string[0x41];
    bool OS64bit;
    bool Mode64bit;

	bool bAtom;
    bool bLongMode;

    // Call Detect()
    explicit CPUInfo();

   private:
    // Detects the various cpu features
    void Detect();
};

	extern CPUInfo cpu_info;
};  // namespace gui::help