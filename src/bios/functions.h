#pragma once
#include <stdint.h>
#include "cop0.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace bios {
struct Function {
    const char* name;
    int argc;
    std::function<bool(mips::CPU& cpu)> callback;

    Function(const char* name, int argc) : name(name), argc(argc), callback(nullptr) {}
    Function(const char* name, int argc, std::function<bool(mips::CPU& cpu)> callback) : name(name), argc(argc), callback(callback) {}
};

inline bool noLog(mips::CPU& cpu) { return false; }

inline bool dbgOutputChar(mips::CPU& cpu) {
    putchar(cpu.reg[4]);
    return false;  // Do not log function call
}

inline bool haltSystem(mips::CPU& cpu) {
    cpu.state = mips::CPU::State::halted;
    return true;
}

const std::unordered_map<uint8_t, Function> A0 = {
    {0x00, {"FileOpen", 2}},           // filename,accessmode
    {0x01, {"FileSeek", 3}},           // fd,offset,seektype
    {0x02, {"FileRead", 3}},           // fd,dst,length
    {0x03, {"FileWrite", 3}},          // fd,src,length
    {0x04, {"FileClose", 1}},          // fd
    {0x05, {"FileIoctl", 3}},          // fd,cmd,arg
    {0x06, {"exit", 1}},               // exitcode
    {0x07, {"FileGetDeviceFlag", 1}},  // fd
    {0x08, {"FileGetc", 1}},           // fd
    {0x09, {"FilePutc", 2}},           // char,fd
    {0x0A, {"todigit", 1}},            // char
    {0x0B, {"atof", 1}},               // src     ;Does NOT work - uses ABSENT cop1 !!!
    {0x0C, {"strtoul", 3}},            // src,src_end,base
    {0x0D, {"strtol", 3}},             // src,src_end,base
    {0x0E, {"abs", 1}},                // val
    {0x0F, {"labs", 1}},               // val
    {0x10, {"atoi", 1}},               // src
    {0x11, {"atol", 1}},               // src
    {0x12, {"atob", 2}},               // src,num_dst
    {0x13, {"SaveState", 1}},          // buf
    {0x14, {"RestoreState", 2}},       // buf,param
    {0x15, {"strcat", 2}},             // dst,src
    {0x16, {"strncat", 3}},            // dst,src,maxlen
    {0x17, {"strcmp", 2}},             // str1,str2
    {0x18, {"strncmp", 3}},            // str1,str2,maxlen
    {0x19, {"strcpy", 2}},             // dst,src
    {0x1A, {"strncpy", 3}},            // dst,src,maxlen
    {0x1B, {"strlen", 1}},             // src
    {0x1C, {"index", 2}},              // src,char
    {0x1D, {"rindex", 2}},             // src,char
    {0x1E, {"strchr", 2}},             // src,char  ;exactly the same as "index"
    {0x1F, {"strrchr", 2}},            // src,char ;exactly the same as "rindex"
    {0x20, {"strpbrk", 2}},            // src,list
    {0x21, {"strspn", 2}},             // src,list
    {0x22, {"strcspn", 2}},            // src,list
    {0x23, {"strtok", 2}},             // src,list
    {0x24, {"strstr", 2}},             // str,substr - buggy
    {0x25, {"toupper", 1}},            // char
    {0x26, {"tolower", 1}},            // char
    {0x27, {"bcopy", 3}},              // src,dst,len
    {0x28, {"bzero", 2}},              // dst,len
    {0x29, {"bcmp", 3}},               // ptr1,ptr2,len      ;Bugged
    {0x2A, {"memcpy", 3}},             // dst,src,len
    {0x2B, {"memset", 3}},             // dst,fillbyte,len
    {0x2C, {"memmove", 3}},            // dst,src,len     ;Bugged
    {0x2D, {"memcmp", 3}},             // src1,src2,len    ;Bugged
    {0x2E, {"memchr", 3}},             // src,scanbyte,len
    {0x2F, {"rand", 0}},
    {0x30, {"srand", 1}},            // seed
    {0x31, {"qsort", 4}},            // base,nel,width,callback
    {0x32, {"strtod", 2}},           // src,src_end   //ABSENT cop1 !!!
    {0x33, {"malloc", 1}},           // size
    {0x34, {"free", 1}},             // buf
    {0x35, {"lsearch", 4}},          // key,base,nel,width,callback
    {0x36, {"bsearch", 4}},          // key,base,nel,width,callback
    {0x37, {"calloc", 2}},           // sizx,sizy            ;SLOW!s
    {0x38, {"realloc", 2}},          // old_buf,new_siz     ;SLOW!
    {0x39, {"InitHeap", 2}},         // addr,size
    {0x3A, {"SystemErrorExit", 1}},  // exitcode
    {0x3B, {"std_in_getchar", 0}},
    {0x3C, {"std_out_putchar", 1, dbgOutputChar}},  // char
    {0x3D, {"std_in_gets", 1}},                     // dst
    {0x3E, {"std_out_puts", 1}},                    // src
    {0x3F, {"printf", 4, noLog}},                   // txt,param1,param2,etc.
    {0x40, {"SystemErrorUnresolvedException", 0, haltSystem}},
    {0x41, {"LoadExeHeader", 2}},  // filename,headerbuf
    {0x42, {"LoadExeFile", 2}},    // filename,headerbuf
    {0x43, {"DoExecute", 3}},      // headerbuf,param1,param2
    {0x44, {"FlushCache", 0}},
    {0x45, {"init_a0_b0_c0_vectors", 0}},
    {0x46, {"GPU_dw", 5}},                // Xdst,Ydst,Xsiz,Ysiz,src
    {0x47, {"gpu_send_dma", 5}},          // Xdst,Ydst,Xsiz,Ysiz,src
    {0x48, {"SendGP1Command", 1}},        // gp1cmd
    {0x49, {"GPU_cw", 1}},                // gp0cmd   ;send GP0 command word
    {0x4A, {"GPU_cwp", 2}},               // src,num ;send GP0 command word and parameter words
    {0x4B, {"send_gpu_linked_list", 1}},  // src
    {0x4C, {"gpu_abort_dma", 0}},
    {0x4D, {"GetGPUStatus", 0}},
    {0x4E, {"gpu_sync", 0}},
    {0x51, {"LoadAndExecute", 3}},  // filename,stackbase,stackoffset
    {0x54, {"CdInit", 0}},
    {0x55, {"_bu_init", 0}},
    {0x56, {"CdRemove", 0}},            //  ;does NOT work due to SysDeqIntRP bug
    {0x5B, {"dev_tty_init", 0}},        //                                      ;PS2: SystemError
    {0x5C, {"dev_tty_open", 3}},        // fcb,and unused:"path\name",accessmode ;PS2: SystemError
    {0x5D, {"dev_tty_in_out", 2}},      // fcb,cmd                             ;PS2: SystemError
    {0x5E, {"dev_tty_ioctl", 3}},       // fcb,cmd,arg                          ;PS2: SystemError
    {0x5F, {"dev_cd_open", 3}},         // fcb,"path\name",accessmode
    {0x60, {"dev_cd_read", 3}},         // fcb,dst,len
    {0x61, {"dev_cd_close", 1}},        // fcb
    {0x62, {"dev_cd_firstfile", 3}},    // fcb,"path\name",direntry
    {0x63, {"dev_cd_nextfile", 2}},     // fcb,direntry
    {0x64, {"dev_cd_chdir", 2}},        // fcb,"path"
    {0x65, {"dev_card_open", 3}},       // fcb,"path\name",accessmode
    {0x66, {"dev_card_read", 3}},       // fcb,dst,len
    {0x67, {"dev_card_write", 3}},      // fcb,src,len
    {0x68, {"dev_card_close", 1}},      // fcb
    {0x69, {"dev_card_firstfile", 3}},  // fcb,"path\name",direntry
    {0x6A, {"dev_card_nextfile", 2}},   // fcb,direntry
    {0x6B, {"dev_card_erase", 2}},      // fcb,"path\name"
    {0x6C, {"dev_card_undelete", 2}},   // fcb,"path\name"
    {0x6D, {"dev_card_format", 1}},     // fcb
    {0x6E, {"dev_card_rename", 4}},     // fcb1,"path\name1",fcb2,"path\name2"
    {0x70, {"_bu_init", 0}},
    {0x71, {"CdInit", 0}},
    {0x72, {"CdRemove", 0}},           //   ;does NOT work due to SysDeqIntRP bug
    {0x78, {"CdAsyncSeekL", 1}},       // src
    {0x7C, {"CdAsyncGetStatus", 1}},   // dst
    {0x7E, {"CdAsyncReadSector", 3}},  // count,dst,mode
    {0x81, {"CdAsyncSetMode", 1}},     // mode
    {0x90, {"CdromIoIrqFunc1", 0}},
    {0x91, {"CdromDmaIrqFunc1", 0}},
    {0x92, {"CdromIoIrqFunc2", 0}},
    {0x93, {"CdromDmaIrqFunc2", 0}},
    {0x94, {"CdromGetInt5errCode", 2}},  // dst1,dst2
    {0x95, {"CdInitSubFunc", 0}},
    {0x96, {"AddCDROMDevice", 0}},
    {0x97, {"AddMemCardDevice", 0}},   //     ;DTL-H: SystemError
    {0x98, {"AddDuartTtyDevice", 0}},  //    ;DTL-H: AddAdconsTtyDevice ;PS2: SystemError
    {0x99, {"AddDummyTtyDevice", 0}},
    {0x9C, {"SetConf", 3}},                 // num_EvCB,num_TCB,stacktop
    {0x9D, {"GetConf", 3}},                 // num_EvCB_dst,num_TCB_dst,stacktop_dst
    {0x9E, {"SetCdromIrqAutoAbort", 2}},    // type,flag
    {0x9F, {"SetMemSize", 1}},              // megabytes
    {0xA1, {"BootFailed", 0, haltSystem}},  // Called when booting CD fails
};

const std::unordered_map<uint8_t, Function> B0 = {
    {0x00, {"alloc_kernel_memory", 1}},  // size,
    {0x01, {"free_kernel_memory", 1}},   // buf,
    {0x02, {"init_timer", 3}},           // t, reload, flags,
    {0x03, {"get_timer", 1}},            // t,
    {0x04, {"enable_timer_irq", 1}},     // t,
    {0x05, {"disable_timer_irq", 1}},    // t,
    {0x06, {"restart_timer", 1}},        // t,
    {0x07, {"DeliverEvent", 2}},         // class, spec,
    {0x08, {"OpenEvent", 4}},            // class, spec, mode, func,
    {0x09, {"CloseEvent", 1}},           // event,
    {0x0A, {"WaitEvent", 1}},            // event,
    {0x0B, {"TestEvent", 1, noLog}},     // event,
    {0x0C, {"EnableEvent", 1}},          // event,
    {0x0D, {"DisableEvent", 1}},         // event,
    {0x0E, {"OpenThread", 3}},           // reg_PC, reg_SP_FP, reg_GP,
    {0x0F, {"CloseThread", 1}},          // handle,
    {0x10, {"ChangeThread", 1}},         // handle,
    {0x11, {"jump_to_00000000h", 0}},
    {0x12, {"InitPad", 4}},  // buf1, siz1, buf2, siz2,
    {0x13, {"StartPad", 0}},
    {0x14, {"StopPad", 0}},
    {0x15, {"OutdatedPadInitAndStart", 4}},  // type, button_dest, unused, unused,
    {0x16, {"OutdatedPadGetButtons", 0}},
    {0x17, {"ReturnFromException", 0}},
    {0x18, {"SetDefaultExitFromException", 0}},
    {0x19, {"SetCustomExitFromException", 1}},  // addr,
    {0x1A, {"SystemError", 0, haltSystem}},
    {0x1B, {"SystemError", 0, haltSystem}},
    {0x1C, {"SystemError", 0, haltSystem}},
    {0x1D, {"SystemError", 0, haltSystem}},
    {0x1E, {"SystemError", 0, haltSystem}},
    {0x1F, {"SystemError", 0, haltSystem}},
    {0x20, {"UnDeliverEvent", 2}},  // class, spec,
    {0x21, {"SystemError", 0, haltSystem}},
    {0x22, {"SystemError", 0, haltSystem}},
    {0x23, {"SystemError", 0, haltSystem}},
    {0x2A, {"SystemError", 0, haltSystem}},
    {0x2B, {"SystemError", 0, haltSystem}},
    {0x32, {"FileOpen", 2}},           // filename, accessmode,
    {0x33, {"FileSeek", 3}},           // fd, offset, seektype,
    {0x34, {"FileRead", 3}},           // fd, dst, length,
    {0x35, {"FileWrite", 3}},          // fd, src, length,
    {0x36, {"FileClose", 1}},          // fd,
    {0x37, {"FileIoctl", 3}},          // fd, cmd, arg,
    {0x38, {"exit", 1}},               // exitcode,
    {0x39, {"FileGetDeviceFlag", 1}},  // fd,
    {0x3A, {"FileGetc", 1}},           // fd,
    {0x3B, {"FilePutc", 2}},           // char, fd,
    {0x3C, {"std_in_getchar", 0}},
    {0x3D, {"std_out_putchar", 1, dbgOutputChar}},  // char,
    {0x3E, {"std_in_gets", 1}},                     // dst,
    {0x3F, {"std_out_puts", 1}},                    // src,
    {0x40, {"chdir", 1}},                           // name,
    {0x41, {"FormatDevice", 1}},                    // devicename,
    {0x42, {"firstfile", 2}},                       // filename, direntry,
    {0x43, {"nextfile", 1}},                        // direntry,
    {0x44, {"FileRename", 2}},                      // old_filename, new_filename,
    {0x45, {"FileDelete", 1}},                      // filename,
    {0x46, {"FileUndelete", 1}},                    // filename,
    {0x47, {"AddDevice", 1}},                       // device_info
    {0x48, {"RemoveDevice", 1}},                    // device_name_lowercase,
    {0x49, {"PrintInstalledDevices", 0}},
    {0x4A, {"InitCard", 1}},  // pad_enable
    {0x4B, {"StartCard", 0}},
    {0x4C, {"StopCard", 0}},
    {0x4D, {"_card_info_subfunc", 1}},  // port",
    {0x4E, {"write_card_sector", 3}},   // port, sector, src,
    {0x4F, {"read_card_sector", 3}},    // port, sector, dst,
    {0x50, {"allow_new_card", 0}},
    {0x51, {"Krom2RawAdd", 1}},  // shiftjis_code,
    {0x52, {"SystemError", 0, haltSystem}},
    {0x53, {"Krom2Offset", 1}},  // shiftjis_code,
    {0x54, {"GetLastError", 0}},
    {0x55, {"GetLastFileError", 1}},  // fd,
    {0x56, {"GetC0Table", 0}},
    {0x57, {"GetB0Table", 0}},
    {0x58, {"get_bu_callback_port", 0}},
    {0x59, {"testdevice", 1}},  // devicename,
    {0x5A, {"SystemError", 0, haltSystem}},
    {0x5B, {"ChangeClearPad", 1}},    // int,
    {0x5C, {"get_card_status", 1}},   // slot,
    {0x5D, {"wait_card_status", 1}},  // slot,
};
};