#pragma once

struct PsxExe {
    char magic[8];  // PS-X EXE
    uint32_t text;
    uint32_t data;

    uint32_t pc0;
    uint32_t gp0;

    uint32_t t_addr;
    uint32_t t_size;

    uint32_t d_addr;  // unknown
    uint32_t d_size;  // unknown

    uint32_t b_addr;  // Memfill
    uint32_t b_size;

    uint32_t s_addr;
    uint32_t s_size;

    uint32_t sp, fp, gp, ret, base;  // reserved, should be 0

    char license[60];
};