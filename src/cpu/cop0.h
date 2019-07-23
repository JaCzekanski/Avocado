#pragma once
#include <memory>
#include "device/device.h"

struct COP0 {
    // cop0r7 DCIC - breakpoint control
    union DCIC {
        struct {
            uint32_t breakpointHit : 1;
            uint32_t codeBreakpointHit : 1;
            uint32_t dataBreakpointHit : 1;
            uint32_t dataReadBreakpointHit : 1;
            uint32_t dataWriteBreakpointHit : 1;
            uint32_t jumpBreakpointHit : 1;
            uint32_t : 6;  // not used

            uint32_t jumpRedirection : 2;  // 0 - disabled, 1..3 - enabled

            uint32_t : 2;  // Unknown
            uint32_t : 7;  // not used

            uint32_t superMasterEnable1 : 1;  // bits 24..29

            uint32_t breakOnCode : 1;
            uint32_t breakOnData : 1;
            uint32_t breakOnDataRead : 1;
            uint32_t breakOnDataWrite : 1;
            uint32_t breakOnJump : 1;

            uint32_t masterEnableBreakAnyJump : 1;
            uint32_t masterEnableBreakpoints : 1;  // bits 24..27
            uint32_t superMasterEnable2 : 1;       // bits 24..29
        };

        uint32_t _reg;
        DCIC() : _reg(0) {}
    };
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
            uint32_t branchTaken : 1;        /** When the branchDelay bit is set, the branchTaken Bit determines whether or not the
                                              * branch is taken. A value of one in branchTaken indicates that the branch is
                                              * taken. The Target Address Register holds the return address.
                                              *
                                              * source: L64360 datasheet
                                              */
            uint32_t branchDelay : 1;        /** CPU sets this bit to one to indicate that the last exception
                                              * was taken while executing in a branch delay slot.
                                              *
                                              * source: L64360 datasheet
                                              */
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

        void clearForException() {
            auto _interruptPending = interruptPending;
            _reg = 0;
            interruptPending = _interruptPending;
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

        void enterException() {
            oldInterruptEnable = previousInterruptEnable;
            oldMode = previousMode;

            previousInterruptEnable = interruptEnable;
            previousMode = mode;

            interruptEnable = false;
            mode = COP0::STATUS::Mode::kernel;
        }

        uint32_t _reg;
        STATUS() : _reg(0) {}
    };

    uint32_t bpc = 0;   // r3  - Breakpoint Program Counter
    uint32_t bda = 0;   // r5  - Breakpoint Data Address
    uint32_t tar = 0;   // r6  - Target Address
                        /** When the cause of an exception is in the branch delay slot (BD Bit in the Cause Register is set to one),
                         * execution resumes either at the target of the branch or at the EPC + 8.
                         * If the branch was taken, the BT Bit is set in the Cause Register to one and loads the branch target address in the TAR Register.
                         * The exception handler needs only to load this address into a register and jump to that location.
                         *
                         * source: L64360 datasheet
                         */
    DCIC dcic;          // r7  - Debug and Cache Invalidate Control
    uint32_t bada = 0;  // r8  - Bad Address
    uint32_t bdam = 0;  // r9  - Breakpoint Data Address Mask
    uint32_t bpcm = 0;  // r11 - Breakpoint Program Counter Mask
    STATUS status;      // r12 - Status
    CAUSE cause;        // r13 - Cause
    uint32_t epc = 0;   // r14 - Exception Program Counter
                        /* In most cases, the EPC Register contains the address of the instruction that caused
                         * the exception. However, when the exception instruction resides in a branch
                         * delay slot, the Cause Register’s BD Bit is set to one to indicate that the EPC
                         * Register contains the address of the immediately preceding branch or jump instruction.
                         *
                         * source: L64360 datasheet
                         */
    uint32_t prid = 2;  // r15 - Processor Revision Identifier

    std::pair<uint32_t, bool> read(int reg);
    void write(int reg, uint32_t value);
    void returnFromException();
};