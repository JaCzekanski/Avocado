#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>
#include "debugger/debugger.h"
#include "system.h"
#include "utils/log.h"
#include "utils/string.h"

namespace bios {

enum class Type {
    INT,     // 32bit HEX - int
    CHAR,    // 8bit char - char
    STRING,  // 32bit pointer to string - char*
    POINTER
};

struct Arg {
    Type type;
    std::string name;
};

struct Function {
    std::string name;
    std::vector<Arg> args;
    std::function<bool(System* sys)> callback;

    Function(const std::string& prototype, std::function<bool(System* sys)> callback = nullptr) : callback(callback) {
        auto argStart = prototype.find('(');
        auto argEnd = prototype.find(')');

        assert(argStart != std::string::npos && argEnd != std::string::npos);
        std::string sArgs = prototype.substr(argStart + 1, argEnd - argStart - 1);
        std::vector<std::string> args = split(sArgs, ", ");  // TODO: do trimming

        std::string sFuncName = prototype.substr(0, argStart);

        this->name = sFuncName.c_str();
        for (auto sArg : args) {
            sArg = trim(sArg);
            auto delim = sArg.find_last_of(' ');

            if (delim == std::string::npos) {
                throw std::runtime_error("Invalid parameter without type");
            }

            auto sType = trim(sArg.substr(0, delim));
            auto sName = trim(sArg.substr(delim + 1));

            Arg arg;
            arg.name = sName;
            arg.type = Type::INT;

            if (sType == "int")
                arg.type = Type::INT;
            else if (sType == "char")
                arg.type = Type::CHAR;
            else if (sType == "const char*" || sType == "char*")
                arg.type = Type::STRING;
            else if (sType == "FILE*")
                arg.type = Type::INT;
            else if (sType == "void*")
                arg.type = Type::POINTER;
            else
                throw std::runtime_error(string_format("%s -> Invalid parameter type", prototype.c_str()).c_str());

            this->args.push_back(arg);
        }
    }
};

inline bool noLog(System* sys) {
    (void)(sys);
    return false;
}

inline bool dbgOutputChar(System* sys) {
    if (sys->debugOutput) putchar(sys->cpu->reg[4]);
    return false;  // Do not log function call
}

inline bool dbgOutputString(System* sys) {
    if (!sys->debugOutput) return false;
    for (int i = 0; i < 80; i++) {
        char c = sys->readMemory8(sys->cpu->reg[4] + i);
        if (c == 0) {
            printf("\n");
            return false;
        }
        putchar(c);
    }
    return false;  // Do not log function call
}

inline bool haltSystem(System* sys) {
    sys->state = System::State::halted;
    return true;
}

inline bool unresolvedException(System* sys) {
    const int howManyInstructionsToDisassemble = 6;
    auto cause = sys->cpu->cop0.cause;
    uint32_t epc = sys->cpu->cop0.epc;

    log::printf("🔴Unresolved exception⚪️: 🅱️%s❌‍⚪️ (%u), epc=🔵0x%08x⚪️, ra=🔵0x%08x\n",
                cause.getExceptionName(), cause.exception, epc, sys->cpu->reg[31]);
    for (uint32_t addr = epc - howManyInstructionsToDisassemble * 4; addr <= epc; addr += 4) {
        auto opcode = mips::Opcode(sys->readMemory32(addr));
        auto ins = debugger::decodeInstruction(opcode);

        if (addr == epc) {
            ins.parameters += "🅱️     <---- Caused the exception";
        }

        log::printf("🔵0x%08x:⚪️ %-8s %s\n", addr, ins.mnemonic.c_str(), ins.parameters.c_str());
    }
    log::printf("🔴This is most likely bug in Avocado, please report it.\n");
    log::printf("🔴 🅱️Emulation stopped.\n");

    sys->state = System::State::halted;
    return false;
}

namespace {
const std::unordered_map<uint8_t, Function> A0 = {
    {0x00, {"FileOpen(const char* file, int mode)"}},
    {0x01, {"FileSeek(FILE* file, int offset, int origin)"}},
    {0x02, {"FileRead(FILE* file, void* dst, int length)"}},
    {0x03, {"FileWrite(FILE* file, void* src, int length)"}},
    {0x04, {"FileClose(FILE* file)"}},
    {0x05, {"FileIoctl(FILE* file, int cmd, int arg)"}},
    {0x06, {"exit(int exitcode)"}},
    {0x07, {"FileGetDeviceFlag(FILE* file)"}},
    {0x08, {"FileGetc(FILE* file)"}},
    {0x09, {"FilePutc(char c, FILE* file)"}},
    {0x0A, {"todigit(char c)"}},
    {0x0B, {"atof(int src)"}},  // ;Does NOT work - uses ABSENT cop1 !!!
    {0x0C, {"strtoul(const char* src, int src_end, int base)"}},
    {0x0D, {"strtol(const char* src, int src_end, int base)"}},
    {0x0E, {"abs(int val)"}},
    {0x0F, {"labs(int val)"}},
    {0x10, {"atoi(int src)"}},
    {0x11, {"atol(int src)"}},
    {0x12, {"atob(const char* src, void* num_dst)"}},
    {0x13, {"SaveState(void* buf)"}},
    {0x14, {"RestoreState(void* buf, int param)"}},
    {0x15, {"strcat(char* dest, const char* src)"}},
    {0x16, {"strncat(char* dest, const char* src, int maxlen)"}},
    {0x17, {"strcmp(const char* str1, const char* str2)"}},
    {0x18, {"strncmp(const char* str1, const char* str2, int maxlen)"}},
    {0x19, {"strcpy(char* dst, const char* src)"}},
    {0x1A, {"strncpy(char* dst, const char* src, int maxlen)"}},
    {0x1B, {"strlen(const char* src)"}},
    {0x1C, {"index(const char* src, char c)"}},
    {0x1D, {"rindex(const char* src, char c)"}},
    {0x1E, {"strchr(const char* src, char c)"}},
    {0x1F, {"strrchr(const char* src, char c)"}},
    {0x20, {"strpbrk(const char* src, const char* list"}},
    {0x21, {"strspn(const char* src, const char* list"}},
    {0x22, {"strcspn(const char* src, const char* list"}},
    {0x23, {"strtok(const char* src, const char* list"}},
    {0x24, {"strstr(const char* str, const char* substr"}},  // buggy
    {0x25, {"toupper(char c)"}},
    {0x26, {"tolower(char c)"}},
    {0x27, {"bcopy(void* src, void* dst, int len)"}},
    {0x28, {"bzero(void* dst, int len)"}},
    {0x29, {"bcmp(void* ptr1, void* ptr2, int len)"}},  // ;Bugged
    {0x2A, {"memcpy(void* dst, void* src, int len"}},
    {0x2B, {"memset(void* dst, int value, int num)"}},
    {0x2C, {"memmove(void* dst, void* src, int len)"}},   // ;Bugged
    {0x2D, {"memcmp(void* src1, void* src2, int len)"}},  // ;Bugged
    {0x2E, {"memchr(void* ptr, char value, int num)"}},
    {0x2F, {"rand()"}},
    {0x30, {"srand(int seed)"}},
    {0x31, {"qsort(void* base, int num, int size, void* callback)"}},
    {0x32, {"strtod(const char* str, int endptr)"}},  // ABSENT cop1 !!!
    {0x33, {"malloc(int size)"}},
    {0x34, {"free(void* ptr)"}},
    {0x35, {"lsearch(void* key, void* base, int num, int size, void* callback)"}},
    {0x36, {"bsearch(void* key, void* base, int num, int size, void* callback)"}},
    {0x37, {"calloc(int num, int size)"}},     // ;SLOW!
    {0x38, {"realloc(void* ptr, int size)"}},  // ;SLOW!
    {0x39, {"InitHeap(void* addr, int size)"}},
    {0x3A, {"SystemErrorExit(int code)"}},
    {0x3B, {"std_in_getchar()"}},
    {0x3C, {"std_out_putchar(char c)", dbgOutputChar}},
    {0x3D, {"std_in_gets(void* dst)"}},
    {0x3E, {"std_out_puts(const char* src)", dbgOutputString}},
    {0x3F, {"printf(const char* fmt)"}},
    {0x40, {"SystemErrorUnresolvedException()", unresolvedException}},
    {0x41, {"LoadExeHeader(const char* filename, void* headerbuf)"}},
    {0x42, {"LoadExeFile(const char* filename, void* headerbuf)"}},
    {0x43, {"DoExecute(void* header, int param1, int param2)"}},
    {0x44, {"FlushCache()"}},
    {0x45, {"init_a0_b0_c0_vectors()"}},
    {0x46, {"GPU_dw(int xdst, int ydst, int xsiz, int ysiz, void* src)"}},
    {0x47, {"gpu_send_dma(int xdst, int ydst, int xsiz, int ysiz, void* src)"}},
    {0x48, {"SendGP1Command(int cmd)"}},
    {0x49, {"GPU_cw(int cmd)"}},
    {0x4A, {"GPU_cwp(void* src, int num)"}},
    {0x4B, {"send_gpu_linked_list(void* src)"}},
    {0x4C, {"gpu_abort_dma()"}},
    {0x4D, {"GetGPUStatus()"}},
    {0x4E, {"gpu_sync()"}},
    {0x51, {"LoadAndExecute(const char* filename, int stackbase, int stackoffset)"}},
    {0x54, {"CdInit()"}},
    {0x55, {"_bu_init()"}},
    {0x56, {"CdRemove()"}},                                              //  ;does NOT work due to SysDeqIntRP bug
    {0x5B, {"dev_tty_init()"}},                                          //                                      ;PS2: SystemError
    {0x5C, {"dev_tty_open(int fcb, const char* path, int accessmode"}},  // ;PS2: SystemError
    {0x5D, {"dev_tty_in_out(int fcb, int cmd)"}},                        // ;PS2: SystemError
    {0x5E, {"dev_tty_ioctl(int fcb, int cmd, int arg)"}},                // ;PS2: SystemError
    {0x5F, {"dev_cd_open(int fcb, const char* path, int accessmode)"}},
    {0x60, {"dev_cd_read(int fcb, void* dst, int len)"}},
    {0x61, {"dev_cd_close(int fcb)"}},
    {0x62, {"dev_cd_firstfile(int fcb, const char* path, void* direntry)"}},
    {0x63, {"dev_cd_nextfile(int fcb, void* direntry)"}},
    {0x64, {"dev_cd_chdir(int fcb, const char* path)"}},
    {0x65, {"dev_card_open(int fcb, const char* path, int accessmode)"}},
    {0x66, {"dev_card_read(int fcb, void* dst, int len)"}},
    {0x67, {"dev_card_write(int fcb, void* src, int len)"}},
    {0x68, {"dev_card_close(int fcb)"}},
    {0x69, {"dev_card_firstfile(int fcb, const char* path, void* direntry)"}},
    {0x6A, {"dev_card_nextfile(int fcb, void* direntry)"}},
    {0x6B, {"dev_card_erase(int fcb, const char* path)"}},
    {0x6C, {"dev_card_undelete(int fcb, const char* path)"}},
    {0x6D, {"dev_card_format(int fcb)"}},
    {0x6E, {"dev_card_rename(int fcb1, const char* path1, int fcb2, const char* path2)"}},
    {0x70, {"_bu_init()"}},
    {0x71, {"CdInit()"}},
    {0x72, {"CdRemove()"}},               //   ;does NOT work due to SysDeqIntRP bug
    {0x78, {"CdAsyncSeekL(void* src)"}},  // TODO: Implement BCD struct
    {0x7C, {"CdAsyncGetStatus(void* dst)"}},
    {0x7E, {"CdAsyncReadSector(int count, void* dst, int mode)"}},
    {0x81, {"CdAsyncSetMode(int mode)"}},
    {0x90, {"CdromIoIrqFunc1()"}},
    {0x91, {"CdromDmaIrqFunc1()"}},
    {0x92, {"CdromIoIrqFunc2()"}},
    {0x93, {"CdromDmaIrqFunc2()"}},
    {0x94, {"CdromGetInt5errCode(void* dst1, void* dst2)"}},
    {0x95, {"CdInitSubFunc()"}},
    {0x96, {"AddCDROMDevice()"}},
    {0x97, {"AddMemCardDevice()"}},   //     ;DTL-H: SystemError
    {0x98, {"AddDuartTtyDevice()"}},  //    ;DTL-H: AddAdconsTtyDevice ;PS2: SystemError
    {0x99, {"AddDummyTtyDevice()"}},
    {0x9C, {"SetConf(int numb_EvCB, int numb_TCB, int stacktop)"}},
    {0x9D, {"GetConf(int numb_EvCB, int numb_TCB, void* stacktop_dst)"}},
    {0x9E, {"SetCdromIrqAutoAbort(int type, int flag)"}},
    {0x9F, {"SetMemSize(int size)"}},
    {0xA1, {"BootFailed()", haltSystem}},  // Called when booting CD fails
};

const std::unordered_map<uint8_t, Function> B0 = {
    {0x00, {"alloc_kernel_memory(int size)"}},
    {0x01, {"free_kernel_memory(void* ptr)"}},
    {0x02, {"init_timer(int t, int reload, int flags)"}},
    {0x03, {"get_timer(int t)"}},
    {0x04, {"enable_timer_irq(int t)"}},
    {0x05, {"disable_timer_irq(int t)"}},
    {0x06, {"restart_timer(int t)"}},
    {0x07, {"DeliverEvent(int event, int spec)"}},
    {0x08, {"OpenEvent(int class, int spec, int mode, void* func)"}},
    {0x09, {"CloseEvent(int event)"}},
    {0x0A, {"WaitEvent(int event)"}},
    {0x0B, {"TestEvent(int event)"}},
    {0x0C, {"EnableEvent(int event)"}},
    {0x0D, {"DisableEvent(int event)"}},
    {0x0E, {"OpenThread(int pc, int sp_fp, int gp)"}},
    {0x0F, {"CloseThread(int handle)"}},
    {0x10, {"ChangeThread(int handle)"}},
    {0x11, {"jump_to_00000000h()"}},
    {0x12, {"InitPad(void* buf1, int size1, void* buf2, int size2)"}},
    {0x13, {"StartPad()"}},
    {0x14, {"StopPad()"}},
    {0x15, {"OutdatedPadInitAndStart(int type, void* button_dest, int unused1, int unused2)"}},
    {0x16, {"OutdatedPadGetButtons()"}},
    {0x17, {"ReturnFromException()"}},
    {0x18, {"SetDefaultExitFromException()"}},
    {0x19, {"SetCustomExitFromException(int addr)"}},
    {0x1A, {"SystemError()", haltSystem}},
    {0x1B, {"SystemError()", haltSystem}},
    {0x1C, {"SystemError()", haltSystem}},
    {0x1D, {"SystemError()", haltSystem}},
    {0x1E, {"SystemError()", haltSystem}},
    {0x1F, {"SystemError()", haltSystem}},
    {0x20, {"UnDeliverEvent(int event, int spec)"}},
    {0x21, {"SystemError()", haltSystem}},
    {0x22, {"SystemError()", haltSystem}},
    {0x23, {"SystemError()", haltSystem}},
    {0x2A, {"SystemError()", haltSystem}},
    {0x2B, {"SystemError()", haltSystem}},
    {0x32, {"FileOpen(const char* file, int mode)"}},
    {0x33, {"FileSeek(FILE* file, int offset, int origin)"}},
    {0x34, {"FileRead(FILE* file, void* dst, int length)"}},
    {0x35, {"FileWrite(FILE* file, void* src, int length)"}},
    {0x36, {"FileClose(FILE* file)"}},
    {0x37, {"FileIoctl(FILE* file, int cmd, int arg)"}},
    {0x38, {"exit(int code)"}},
    {0x39, {"FileGetDeviceFlag(FILE* file)"}},
    {0x3A, {"FileGetc(FILE* file)"}},
    {0x3B, {"FilePutc(char c, FILE* file)"}},
    {0x3C, {"std_in_getchar()"}},
    {0x3D, {"std_out_putchar(char c)", dbgOutputChar}},
    {0x3E, {"std_in_gets(void* dst)"}},
    {0x3F, {"std_out_puts(const char* src)", dbgOutputString}},
    {0x40, {"chdir(const char* name)"}},
    {0x41, {"FormatDevice(const char* device_name)"}},
    {0x42, {"firstfile(const char* filename, void* direntry)"}},
    {0x43, {"nextfile(void* direntry)"}},
    {0x44, {"FileRename(const char* old_filename, const char* new_filename)"}},
    {0x45, {"FileDelete(const char* filename)"}},
    {0x46, {"FileUndelete(const char* filename)"}},
    {0x47, {"AddDevice(void* device_info)"}},
    {0x48, {"RemoveDevice(const char* device_name)"}},
    {0x49, {"PrintInstalledDevices()"}},
    {0x4A, {"InitCard(int pad_enable)"}},
    {0x4B, {"StartCard()"}},
    {0x4C, {"StopCard()"}},
    {0x4D, {"_card_info_subfunc(int port)"}},
    {0x4E, {"write_card_sector(int port, int sector, void* src)"}},
    {0x4F, {"read_card_sector(int port, int sector, void* dst)"}},
    {0x50, {"allow_new_card()"}},
    {0x51, {"Krom2RawAdd(int shiftjis_code)"}},
    {0x52, {"SystemError()", haltSystem}},
    {0x53, {"Krom2Offset(int shiftjis_code)"}},
    {0x54, {"GetLastError()"}},
    {0x55, {"GetLastFileError(FILE* file)"}},
    {0x56, {"GetC0Table()"}},
    {0x57, {"GetB0Table()"}},
    {0x58, {"get_bu_callback_port()"}},
    {0x59, {"testdevice(const char* device_name)"}},
    {0x5A, {"SystemError()", haltSystem}},
    {0x5B, {"ChangeClearPad(int unknown)"}},
    {0x5C, {"get_card_status(int slot)"}},
    {0x5D, {"wait_card_status(int slot)"}},
};

const std::unordered_map<uint8_t, Function> C0 = {  //
    {0x00, {"EnqueueTimerAndVblankIrqs(int priority)"}},
    {0x01, {"EnqueueSyscallHandler(int priority)"}},
    {0x02, {"SysEnqIntRP(int priority, void* struc)"}},  // ;bugged, use with care
    {0x03, {"SysDeqIntRP(int priority, void* struc)"}},  // ;bugged, use with care
    {0x04, {"get_free_EvCB_slot()"}},
    {0x05, {"get_free_TCB_slot()"}},
    {0x06, {"ExceptionHandler()"}},
    {0x07, {"InstallExceptionHandlers()"}},  //   ;destroys/uses k0/k1
    {0x08, {"SysInitMemory(void* addr, int size)"}},
    {0x09, {"SysInitKernelVariables()"}},
    {0x0A, {"ChangeClearRCnt(int t, int flag)"}},
    {0x0B, {"SystemError()", haltSystem}},  //  ;PS2: return 0
    {0x0C, {"InitDefInt(int priority)"}},
    {0x0D, {"SetIrqAutoAck(int irq, int flag)"}},
    {0x0E, {"return_0()"}},  //               ;DTL-H2000: dev_sio_init
    {0x0F, {"return_0()"}},  //               ;DTL-H2000: dev_sio_open
    {0x10, {"return_0()"}},  //               ;DTL-H2000: dev_sio_in_out
    {0x11, {"return_0()"}},  //               ;DTL-H2000: dev_sio_ioctl
    {0x12, {"InstallDevices(int ttyflag)"}},
    {0x13, {"FlushStdInOutPut()"}},
    {0x14, {"return 0()"}},  //               ;DTL-H2000: SystemError
    {0x15, {"tty_cdevinput(int circ, char c)"}},
    {0x16, {"tty_cdevscan()"}},
    {0x17, {"tty_circgetc(int circ)"}},  //    ;uses r5 as garbage txt for ioabort
    {0x18, {"tty_circputc(char c, int circ)"}},
    {0x19, {"ioabort(const char* txt1, const char* txt2)"}},
    {0x1A, {"set_card_find_mode(int mode)"}},  // ;0=normal, 1=find deleted files
    {0x1B, {"KernelRedirect(int ttyflag)"}},   // ;PS2: ttyflag=1 causes SystemError
    {0x1C, {"AdjustA0Table()"}},
    {0x1D, {"get_card_find_mode()"}}};
};  // namespace

const std::array<std::unordered_map<uint8_t, Function>, 3> tables = {{A0, B0, C0}};

const std::unordered_map<uint8_t, Function> SYSCALL = {  //
    {0x00, {"NoFunction()"}},
    {0x01, {"EnterCriticalSection()"}},
    {0x02, {"ExitCriticalSection()"}},
    {0x03, {"ChangeThreadSubFunction(int addr)"}},
    {0x04, {"DeliverEvent()"}}};

};  // namespace bios
