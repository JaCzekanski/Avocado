#pragma once
enum RenderingMode {
    software = 1 << 0,
    hardware = 1 << 1,
    mixed = software | hardware,
};