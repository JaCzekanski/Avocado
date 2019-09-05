# Avocado
A Modern Playstation 1 emulator.

<p align="center">
    <img src="docs/boot.png" height="200">
</p>

If you have any questions just catch me on Twitter ([@JaCzekanski](https://twitter.com/JaCzekanski)) or create an [Issue](https://github.com/JaCzekanski/Avocado/issues). There is also [Discord server](https://discord.gg/ATEJApR).

## Compatibility

See [Avocado compatibility list](https://avocado-db.czekanski.info)

## Changelog

*5.09.2019* - Vibration support

*2.09.2019* - Anti-Modchip and LibCrypt protected games support

*13.03.2019* - merged MDEC (video decoder) support

*8.12.2018* - created Compatibility list webapp

*19.11.2018* - added .chd file support

*9.11.2018* - XA-ADPCM (streaming audio) and Red Book audio support - music should play in most games, SPU samples interpolation (sounds less harsh), Pitch modulation support (used by few games), refactored CDROM reading and cue parsing subsystems, various fixes, hardware (OpenGL) rendering

*27.08.2018* - Support for digital controller, DualShock (without vibrations) and mouse. Two controllers are supported (no multitap for now)

*19.08.2018* - Memory card support

*11.08.2018* - Partial SPU implementation - sound working

## Status

Full list: [Avocado Builds](https://avocado-builds.czekanski.info)

Build   | Status | Download
--------|--------|---------
Linux | [![Build Status](https://avocado-builds.czekanski.info/status/linux64)](https://travis-ci.org/JaCzekanski/Avocado) | [develop](https://avocado-builds.czekanski.info/latest/linux64)
macOS | [![Build Status](https://avocado-builds.czekanski.info/status/macos)](https://travis-ci.org/JaCzekanski/Avocado) | [develop](https://avocado-builds.czekanski.info/latest/macos)
Android | [![Build Status](https://avocado-builds.czekanski.info/status/android)](https://travis-ci.org/JaCzekanski/Avocado) | [develop](https://avocado-builds.czekanski.info/latest/android)
Windows 64-bit, Visual Studio 2019 | [![Build status](https://avocado-builds.czekanski.info/status/windows64)](https://ci.appveyor.com/project/JaCzekanski/avocado/branch/develop) | **[develop](https://avocado-builds.czekanski.info/latest/windows64)**
Windows 32-bit, Visual Studio 2019 | [![Build status](https://avocado-builds.czekanski.info/status/windows32)](https://ci.appveyor.com/project/JaCzekanski/avocado/branch/develop) | [develop](https://avocado-builds.czekanski.info/latest/windows32)
Windows 64-bit, Visual Studio 2017 | [![Build status](https://ci.appveyor.com/api/projects/status/h1cs3bj1vhskjxgx/branch/develop?svg=true)](https://ci.appveyor.com/project/JaCzekanski/avocado/branch/develop) |
Windows 32-bit, Visual Studio 2017 | [![Build status](https://ci.appveyor.com/api/projects/status/h1cs3bj1vhskjxgx/branch/develop?svg=true)](https://ci.appveyor.com/project/JaCzekanski/avocado/branch/develop) |

Despite this emulator being in early development, some 3D games can run. [Game compatibility list](https://avocado-db.czekanski.info)


Currently audio is hard synced to NTSC (60Hz) (**running PAL games will make audio stutter**). The timer implementation does not function properly (**games fail to boot** or run at wrong speed). Many games won't boot or crash shortly after booting.

## Requirements
- OS: Windows 7 or later, macOS 10.13 or later, Linux
- GPU: Graphics card supporting **OpenGL 3.2**
- CPU: [SSE3 compatible (CPUs after 2004)](https://en.wikipedia.org/wiki/SSE3#CPUs_with_SSE3) x64 or x86 processor

### Notes
Avocado focuses on supporting relatively modern hardware (2010 and forwards) and non-legacy OSes. 

Emulator is currently single-threaded - if you have multicore CPU you will not benefit from it. Single thread performance is what really matters.

Currently Avocado requires OpenGL 3.2. In the future this limitation will be lifted for software rendering.

## Running

Avocado requires the BIOS from real console in the data/bios directory.
Selection of a BIOS rom will be required on the first run. The rom can be changed under Options->BIOS or by modifying the **config.json** file.

Avocado doesn't support fast booting. [UniROM](http://www.psxdev.net/forum/viewtopic.php?t=722) can be used as a work around. Place the .rom file in the data/bios directory and modify **config.json**:
```
"extension": "data/bios/unirom_caetlaNTSC_plugin.rom"
```

Press the **Start** button (Enter by default) to fastboot, or **R2** (keypad *) to slowboot a game.
You can run the included Playstation firmware replacement *Caetla* with the **Select** button (Right shift) then run the .exe directly from the disk.

To load a .cue/.bin/.img/.chd file just drag and drop it.

PAL games with LibCrypt protection need additional subchannel info - download proper file .SBI or .LSD file from [Redump](http://redump.org/discs/system/psx/), place it in the same folder as game image and make sure has identical name as .cue/.bin/... file.

## Controls

- **Space** - pause/resume emulation
- **F1** - hide GUI
- **F2** - soft reset
- **Shift-F2** - hard reset
- **F7** - single frame
- **Tab** - disable framelimiting

Configure controls under Options->Controller menu.

## Build


### Windows
Requirements:
- Visual Studio 2017
- [Premake5](https://premake.github.io/download.html)
- [SDL2 dev library](https://www.libsdl.org/download-2.0.php)

First build: 
```
> git clone https://github.com/JaCzekanski/Avocado.git
> cd Avocado
> git submodule update --init --recursive
> premake5 vs2017

# Open Visual Studio solution and build it
```

See appveyor.yml in case of problems.

Update:
```
> git pull && git submodule update --init --recursive
> premake5 vs2017

# Open Visual Studio solution and build it
```

### Linux
Requirements:
- Clang6 or newer
- [Premake5](https://premake.github.io/download.html)

First build: 
```
> sudo apt update
> sudo apt install libsdl2-dev
> git clone https://github.com/JaCzekanski/Avocado.git
> cd Avocado
> git submodule update --init --recursive
> premake5 gmake
> make config=release_x64 -j4

# Running
> ./build/release_x64/avocado
```

See .travis/linux/build.sh in case of problems.

Update:
```
> git pull && git submodule update --init --recursive
> premake5 gmake
> make config=release_x64 -j4
```

### macOS
Requirements:
- XCode
- [Premake5](https://premake.github.io/download.html)
- [Brew](https://brew.sh)

First build:
```
> brew update
> brew install sdl2
> git clone https://github.com/JaCzekanski/Avocado.git
> cd Avocado
> git submodule update --init --recursive
> premake5 gmake
> make config=release_x64 -j4

# Running
> ./build/release_x64/avocado.app
```

See .travis/macos/build.sh in case of problems.

Update:
```
> git pull && git submodule update --init --recursive
> premake5 gmake
> make config=release_x64 -j4
```

## Bugs

Use [GitHub issue tracker](https://github.com/JaCzekanski/Avocado/issues) to file bugs. Please attach [Game ID](http://redump.org/discs/system/psx/), screenshots/video, BIOS and build version.

See [Game compatibility list](https://avocado-db.czekanski.info) before creating a bug issue.
