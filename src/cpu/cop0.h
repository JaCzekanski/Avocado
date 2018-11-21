#pragma once
#include "device/device.h"

struct COP0 {
    // cop0r13 cause, ro, bit8-9 are rw
    union CAUSE {
        enum class Exception : uint32_t {
            interrupt = 0,
            addressErrorLoad = 4,
            addressErrorStore = 5,
            busErrorInstruction = 6,
            busErrorData = 7,
            syscall = 8,
            breakpoint = 9,
            reservedInstruction = 10,
            coprocessorUnusable = 11,
            arithmeticOverflow = 12
        };

        struct {
            uint32_t : 2;
            Exception exception : 5;
            uint32_t : 1;
            uint32_t interruptPending : 8;
            uint32_t : 12;
            uint32_t coprocessorNumber : 2;  // If coprocessor caused the exception
            uint32_t branchDelayTaken : 1;
            uint32_t isInDelaySlot : 1;
        };

        const char* getExceptionName() const {
            switch (exception) {
                case Exception::interrupt: return "interrupt";
                case Exception::addressErrorLoad: return "addressErrorLoad";
                case Exception::addressErrorStore: return "addressErrorStore";
                case Exception::busErrorInstruction: return "busErrorInstruction";
                case Exception::busErrorData: return "busErrorData";
                case Exception::syscall: return "syscall";
                case Exception::breakpoint: return "breakpoint";
                case Exception::reservedInstruction: return "reservedInstruction";
                case Exception::coprocessorUnusable: return "coprocessorUnusable";
                case Exception::arithmeticOverflow: return "arithmeticOverflow";
                default: return "unknown";
            }
        }

        uint32_t _reg;
        CAUSE() : _reg(0) {}
    };

    // cop0r12 System status, rw
    union STATUS {
        enum class Mode : uint32_t { kernel = 0, user = 1 };
        enum class BootExceptionVectors { ram = 0, rom = 1 };
        struct {
            Bit interruptEnable : 1;
            Mode mode : 1;

            Bit previousInterruptEnable : 1;
            Mode previousMode : 1;

            Bit oldInterruptEnable : 1;
            Mode oldMode : 1;

            uint32_t : 2;

            uint32_t interruptMask : 8;
            Bit isolateCache : 1;
            Bit swappedCache : 1;
            Bit writeZeroAsParityBits : 1;
            Bit : 1;  // CM
            Bit cacheParityError : 1;
            Bit tlbShutdown : 1;  // TS

            BootExceptionVectors bootExceptionVectors : 1;
            uint32_t : 2;
            Bit reverseEndianness : 1;
            uint32_t : 2;

            Bit cop0Enable : 1;
            Bit cop1Enable : 1;
            Bit cop2Enable : 1;
            Bit cop3Enable : 1;
        };

        uint32_t _reg;
        STATUS() : _reg(0) {}
    };

    uint32_t bpc = 0;       // r3
    uint32_t bda = 0;       // r5
    uint32_t jumpdest = 0;  // r6
    uint32_t dcic = 0;      // r7
    uint32_t badVaddr = 0;  // r8
    uint32_t bdam = 0;      // r9
    uint32_t bpcm = 0;      // r11
    STATUS status;          // r12
    CAUSE cause;            // r13
    uint32_t epc = 0;       // r14
    uint32_t revId = 2;     // r15
};