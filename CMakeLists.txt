cmake_minimum_required(VERSION 3.13)
project(Avocado)

set(CMAKE_CXX_STANDARD 17)

option(FORCE_BUILD_SDL "Force build SDL2 from sources." OFF)

set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
add_compile_options(-mavx2 -m64)
add_link_options(-mavx2 -m64)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Building as Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# External libraries

##############################################
# fmt
add_library(fmt STATIC
        externals/fmt/src/format.cc
        )
target_include_directories(fmt PUBLIC externals/fmt/include)

##############################################
# magic_enum
add_library(magic_enum INTERFACE)
target_include_directories(magic_enum INTERFACE externals/magic_enum/include)

##############################################
# event_bus
add_library(event_bus INTERFACE)
target_include_directories(event_bus INTERFACE externals/EventBus/lib/include)

##############################################
# stb_image
add_library(stb_image STATIC
        externals/stb/stb_image.c
        externals/stb/stb_image_write.c
)
target_include_directories(stb_image INTERFACE externals/stb)

##############################################
# cereal
add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE externals/cereal/include)

##############################################
# flac
add_library(flac STATIC
        externals/flac/src/libFLAC/bitmath.c
        externals/flac/src/libFLAC/bitreader.c
        externals/flac/src/libFLAC/cpu.c
        externals/flac/src/libFLAC/crc.c
        externals/flac/src/libFLAC/fixed.c
        externals/flac/src/libFLAC/fixed_intrin_sse2.c
        externals/flac/src/libFLAC/fixed_intrin_ssse3.c
        externals/flac/src/libFLAC/float.c
        externals/flac/src/libFLAC/format.c
        externals/flac/src/libFLAC/lpc.c
        externals/flac/src/libFLAC/lpc_intrin_avx2.c
        externals/flac/src/libFLAC/lpc_intrin_sse2.c
        externals/flac/src/libFLAC/lpc_intrin_sse41.c
        externals/flac/src/libFLAC/lpc_intrin_sse.c
        externals/flac/src/libFLAC/md5.c
        externals/flac/src/libFLAC/memory.c
        externals/flac/src/libFLAC/metadata_iterators.c
        externals/flac/src/libFLAC/metadata_object.c
        externals/flac/src/libFLAC/stream_decoder.c
        externals/flac/src/libFLAC/window.c
        )
target_include_directories(flac
        PRIVATE
        externals/flac/src/libFLAC/include
        PUBLIC
        externals/flac/include
        )
target_compile_definitions(flac
        PRIVATE
        PACKAGE_VERSION="1.3.2"
        FLAC__HAS_OGG=0
        FLAC__NO_DLL
        HAVE_LROUND
        HAVE_STDINT_H
        HAVE_STDLIB_H
        )
# TODO: Add windows unicode

##############################################
# lzma
add_library(lzma STATIC
        externals/lzma/C/Alloc.c
        externals/lzma/C/Bra86.c
        externals/lzma/C/Bra.c
        externals/lzma/C/BraIA64.c
        externals/lzma/C/CpuArch.c
        externals/lzma/C/Delta.c
        externals/lzma/C/LzFind.c
        externals/lzma/C/Lzma86Dec.c
        externals/lzma/C/Lzma86Enc.c
        externals/lzma/C/LzmaDec.c
        externals/lzma/C/LzmaEnc.c
        externals/lzma/C/LzmaLib.c
        externals/lzma/C/Sort.c
        )
target_include_directories(lzma PUBLIC externals/lzma/C)
target_compile_definitions(lzma
        PRIVATE
        _7ZIP_ST
        )

##############################################
# miniz
add_library(miniz STATIC
        externals/miniz/miniz.c
        )
target_include_directories(miniz PUBLIC externals/miniz)
target_link_libraries(miniz)

##############################################
# chdr
add_library(chdr STATIC
        externals/libchdr/src/bitstream.c
        externals/libchdr/src/cdrom.c
        externals/libchdr/src/chd.c
        externals/libchdr/src/huffman.c
        externals/libchdr/src/flac.c
        )
target_include_directories(chdr PUBLIC externals/libchdr/src)
target_link_libraries(chdr flac lzma miniz)
#target_compile_options(chdr PRIVATE
#        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Werror>
#        $<$<CXX_COMPILER_ID:MSVC>:/WX>
#        )

##############################################
# imgui
add_library(imgui STATIC
        externals/imgui/imgui.cpp
        externals/imgui/imgui_draw.cpp
        externals/imgui/imgui_widgets.cpp
        externals/imgui/imgui_tables.cpp
        externals/imgui/misc/cpp/imgui_stdlib.cpp
        )
target_include_directories(imgui PUBLIC externals/imgui/)


##############################################
# glad
add_library(glad STATIC
        externals/glad/src/glad.c
        )
target_include_directories(glad PUBLIC externals/glad/include)


##############################################
# json
add_library(json INTERFACE)
target_include_directories(json INTERFACE externals/json/include)

##############################################
# filesystem
add_library(filesystem INTERFACE)
target_include_directories(filesystem INTERFACE externals/filesystem/include)

##############################################
# SDL
if (FORCE_BUILD_SDL)
    message(STATUS "Force building SDL2 from sources")
    add_subdirectory(externals/SDL2)
else()
    find_package(SDL2)
    if (NOT SDL2_FOUND)
        message(STATUS "SDL2 not found, compiling from source")
        add_subdirectory(externals/SDL2)
    else()
        message(STATUS "SDL2 found")
    endif()
endif()

##############################################
# core
add_library(core STATIC
        src/bios/functions.cpp
        src/config.cpp
        src/cpu/cop0.cpp
        src/cpu/cpu.cpp
        src/cpu/gte/gte.cpp
        src/cpu/gte/math.cpp
        src/cpu/gte/opcodes.cpp
        src/cpu/instructions.cpp
        src/debugger/debugger.cpp
        src/device/cache_control.cpp
        src/device/cdrom/cdrom.cpp
        src/device/cdrom/commands.cpp
        src/device/controller/controller.cpp
        src/device/controller/peripherals/abstract_device.cpp
        src/device/controller/peripherals/analog_controller.cpp
        src/device/controller/peripherals/digital_controller.cpp
        src/device/controller/peripherals/memory_card.cpp
        src/device/controller/peripherals/mouse.cpp
        src/device/controller/peripherals/none.cpp
        src/device/dma/dma.cpp
        src/device/dma/dma0_channel.cpp
        src/device/dma/dma1_channel.cpp
        src/device/dma/dma2_channel.cpp
        src/device/dma/dma3_channel.cpp
        src/device/dma/dma4_channel.cpp
        src/device/dma/dma5_channel.cpp
        src/device/dma/dma6_channel.cpp
        src/device/dma/dma_channel.cpp
        src/device/expansion2.cpp
        src/device/gpu/color_depth.cpp
        src/device/gpu/gpu.cpp
        src/device/gpu/psx_color.cpp
        src/device/gpu/render/dither.cpp
        src/device/gpu/render/render_line.cpp
        src/device/gpu/render/render_rectangle.cpp
        src/device/gpu/render/render_triangle.cpp
        src/device/interrupt.cpp
        src/device/mdec/algorithm.cpp
        src/device/mdec/mdec.cpp
        src/device/memory_control.cpp
        src/device/ram_control.cpp
        src/device/serial.cpp
        src/device/spu/adsr.cpp
        src/device/spu/interpolation.cpp
        src/device/spu/noise.cpp
        src/device/spu/reverb.cpp
        src/device/spu/spu.cpp
        src/device/spu/voice.cpp
        src/device/timer.cpp
        src/disc/disc.cpp
        src/disc/format/chd_format.cpp
        src/disc/format/cue.cpp
        src/disc/format/cue_parser.cpp
        src/disc/format/ecm.cpp
        src/disc/format/ecm_parser.cpp
        src/disc/load.cpp
        src/disc/position.cpp
        src/disc/subchannel_q.cpp
        src/input/input_manager.cpp
        src/sound/adpcm.cpp
        src/sound/tables.cpp
        src/sound/wave.cpp
        src/state/state.cpp
        src/stdafx.cpp
        src/system.cpp
        src/system_tools.cpp
        src/utils/bcd.cpp
        src/utils/event.cpp
        src/utils/gpu_draw_list.cpp
        src/utils/file.cpp
        src/utils/psf.cpp
        src/utils/stb_image_write.cpp
        src/utils/string.cpp
        src/utils/vector.cpp
        )

target_include_directories(core
        PUBLIC
        src
        )

target_link_libraries(core
        fmt
        magic_enum
        event_bus
        stb_image
        cereal
        chdr
        miniz
        )

target_compile_options(core PUBLIC
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

set(SOURCES
        src/imgui/imgui_impl_opengl3.cpp
        src/imgui/imgui_impl_sdl.cpp
        src/platform/windows/config_parser.cpp
        src/platform/windows/file/file.cpp
        src/platform/windows/gui/debug/cdrom.cpp
        src/platform/windows/gui/debug/cpu.cpp
        src/platform/windows/gui/debug/gpu.cpp
        src/platform/windows/gui/debug/gte.cpp
        src/platform/windows/gui/debug/io.cpp
        src/platform/windows/gui/debug/kernel.cpp
        src/platform/windows/gui/debug/spu.cpp
        src/platform/windows/gui/debug/timers.cpp
        src/platform/windows/gui/file/open.cpp
        src/platform/windows/gui/gui.cpp
        src/platform/windows/gui/help/about.cpp
        src/platform/windows/gui/helper/file_dialog.cpp
        src/platform/windows/gui/images.cpp
        src/platform/windows/gui/options/bios.cpp
        src/platform/windows/gui/options/memory_card.cpp
        src/platform/windows/gui/options/options.cpp
        src/platform/windows/gui/options/system_options.cpp
        src/platform/windows/gui/toasts.cpp
        src/platform/windows/input/key.cpp
        src/platform/windows/input/sdl_input_manager.cpp
        src/platform/windows/main.cpp
        src/platform/windows/sound/sound.cpp
        src/platform/windows/utils/platform_tools.cpp
        src/renderer/opengl/opengl.cpp
        src/renderer/opengl/shader/attribute.cpp
        src/renderer/opengl/shader/buffer.cpp
        src/renderer/opengl/shader/framebuffer.cpp
        src/renderer/opengl/shader/program.cpp
        src/renderer/opengl/shader/shader.cpp
        src/renderer/opengl/shader/texture.cpp
        src/renderer/opengl/shader/uniform.cpp
        src/renderer/opengl/shader/vertex_array_object.cpp
        )

add_executable(avocado ${SOURCES})

target_include_directories(avocado
        PRIVATE
        src
        )

target_link_libraries(avocado
        core
        fmt
        imgui
        glad
        json
        SDL2::SDL2
        filesystem
        )

# set_property(TARGET avocado PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
