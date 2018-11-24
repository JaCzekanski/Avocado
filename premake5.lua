function generateVersionFile()
	local version = getSingleLineOutput('git describe --exact-match')
	local branch = getSingleLineOutput('git symbolic-ref --short -q HEAD')
	local commit = getSingleLineOutput('git rev-parse --short=6 HEAD')
	local date = os.date("!%Y-%m-%d %H:%M:%S")

	if os.getenv("APPVEYOR") == "True" then
		branch = os.getenv("APPVEYOR_REPO_BRANCH")
	end

	local versionString = ''

	if version == '' then
		versionString = string.format("%s-%s (%s)", branch, commit, date)
	else
		versionString = string.format("v%s", version)
	end

	versionString = versionString .. " \" BUILD_ARCH \""

	f = io.open('src/version.h', 'w')
	f:write('#pragma once\n')
	f:write(string.format('#ifndef BUILD_ARCH\n'))
	f:write(string.format('    #define BUILD_ARCH "UNKNOWN-ARCH"\n'))
	f:write(string.format('#endif\n'))
	f:write(string.format('#define BUILD_VERSION "%s"\n', version))
	f:write(string.format('#define BUILD_BRANCH "%s"\n', branch))
	f:write(string.format('#define BUILD_COMMIT "%s"\n', commit))
	f:write(string.format('#define BUILD_DATE "%s"\n', date))
	f:write(string.format('#define BUILD_STRING "%s"\n', versionString))
	f:close()
end

function getSingleLineOutput(command)
	return string.gsub(getOutput(command), '\n', '')
end

function getOutput(command)
	local nullDevice = ''
	if package.config:sub(1,1) == '\\' then
		nullDevice = 'NUL'
	else
		nullDevice = '/dev/null'
	end

	local file = io.popen(command .. ' 2> ' .. nullDevice, 'r')
	local output = file:read('*all')
	file:close()

	return output
end

workspace "Avocado"    
	configurations { "debug", "release", "fast_debug" }
	platforms {"x86", "x64"}
    startproject "avocado"
    defaultplatform "x86"

newoption {
	trigger = "disable-load-delay-slots",
	description = "Disable load delay slots (faster execution)",
}
filter "not options:disable-load-delay-slots"
	defines "ENABLE_LOAD_DELAY_SLOTS"

newoption {
	trigger = "enable-io-log",
	description = "Enable IO access log",
}
filter "options:enable-io-log"
	defines "ENABLE_IO_LOG"


filter {}
	language "c++"
	cppdialect "C++17"
	defines { 'BUILD_ARCH="%{cfg.platform}"' }

filter "platforms:x86"
	architecture "x32"

filter "platforms:x64"
	architecture "x64"
	vectorextensions "AVX"

filter "system:macosx"
	xcodebuildsettings {
		['ALWAYS_SEARCH_USER_PATHS'] = {'YES'}
	}


filter {"kind:*App"}
	targetdir "build/%{cfg.buildcfg}_%{cfg.platform}"

filter {"kind:*App", "platforms:x86"}
	targetdir "build/%{cfg.buildcfg}"


filter "configurations:Debug"
	defines { "DEBUG" }
	symbols "On"

filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "Full"

filter "configurations:FastDebug"
    defines { "DEBUG" }
    symbols "On"
    optimize "Speed"
    editandcontinue "On"

filter {"action:vs*"}
    flags { "MultiProcessorCompile" }
	defines "_CRT_SECURE_NO_WARNINGS"

filter {"action:vs*", "configurations:Release"}
    flags {
		"LinkTimeOptimization",
		"StaticRuntime"
	}

filter "action:gmake"
	buildoptions { 
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
	}

project "glad"
	uuid "9add6bd2-2372-4614-a367-2e8863415083"
	kind "StaticLib"
	language "c"
	location "build/libs/glad"
	includedirs { 
		"externals/glad/include"
	}
	files { 
		"externals/glad/src/*.c",
	}

project "miniz"
	uuid "4D28CBE8-3092-4400-B67B-FD51FCAFBD34"
	kind "StaticLib"
	language "c"
	location "build/libs/miniz"
	includedirs { 
		"externals/miniz"
	}
	files { 
		"externals/miniz/*.c",
	}

project "imgui"
	uuid "a8f18b69-f15a-4804-80f7-e8f80ab91369"
	kind "StaticLib"
	location "build/libs/imgui"
	includedirs { 
		"externals/imgui",
		"externals/glad/include",
	}
	files { 
		"externals/imgui/imgui.cpp",
		"externals/imgui/imgui_draw.cpp",
		"externals/imgui/imgui_demo.cpp",
	}

project "flac"
	uuid "2f208b4f-2e69-408e-b0df-b0e72b031b02"
	kind "StaticLib"
	language "c"
	location "build/libs/flac"
	includedirs { 
		"externals/flac/src/libFLAC/include",
		"externals/flac/include",
	}
	files { 
		"externals/flac/src/libFLAC/bitmath.c",
		"externals/flac/src/libFLAC/bitreader.c",
		"externals/flac/src/libFLAC/cpu.c",
		"externals/flac/src/libFLAC/crc.c",
		"externals/flac/src/libFLAC/fixed.c",
		"externals/flac/src/libFLAC/fixed_intrin_sse2.c",
		"externals/flac/src/libFLAC/fixed_intrin_ssse3.c",
		"externals/flac/src/libFLAC/float.c",
		"externals/flac/src/libFLAC/format.c",
		"externals/flac/src/libFLAC/lpc.c",
		"externals/flac/src/libFLAC/lpc_intrin_avx2.c",
		"externals/flac/src/libFLAC/lpc_intrin_sse2.c",
		"externals/flac/src/libFLAC/lpc_intrin_sse41.c",
		"externals/flac/src/libFLAC/lpc_intrin_sse.c",
		"externals/flac/src/libFLAC/md5.c",
		"externals/flac/src/libFLAC/memory.c",
		"externals/flac/src/libFLAC/metadata_iterators.c",
		"externals/flac/src/libFLAC/metadata_object.c",
		"externals/flac/src/libFLAC/stream_decoder.c",
		"externals/flac/src/libFLAC/window.c",
	}
	filter "system:windows" 
		files {
			"externals/flac/src/libFLAC/windows_unicode_filenames.c",
		}
	filter {}
	defines {
		"PACKAGE_VERSION=\"1.3.2\"",
		"FLAC__HAS_OGG=0", 
		"FLAC__NO_DLL", 
		"HAVE_LROUND", 
		"HAVE_STDINT_H", 
		"HAVE_STDLIB_H",
	}


project "lzma"
	uuid "af99f9c5-e14a-4478-bac5-07d457753d35"
	kind "StaticLib"
	language "c"
	location "build/libs/lzma"
	includedirs { 
		"externals/lzma/C",
	}
	files { 
		"externals/lzma/C/Alloc.c",
		"externals/lzma/C/Bra86.c",
		"externals/lzma/C/Bra.c",
		"externals/lzma/C/BraIA64.c",
		"externals/lzma/C/CpuArch.c",
		"externals/lzma/C/Delta.c",
		"externals/lzma/C/LzFind.c",
		"externals/lzma/C/Lzma86Dec.c",
		"externals/lzma/C/Lzma86Enc.c",
		"externals/lzma/C/LzmaDec.c",
		"externals/lzma/C/LzmaEnc.c",
		"externals/lzma/C/LzmaLib.c",
		"externals/lzma/C/Sort.c",
	}
	defines {
		"_7ZIP_ST"
	}

project "chdr"
	uuid "d148de3d-efbb-4f1d-88bc-a112be68fc04"
	kind "StaticLib"
	language "c"
	location "build/libs/chdr"
	includedirs { 
		"externals/flac/include",
		"externals/lzma/C",
		"externals/libchdr/src",
		"externals/miniz",
	}
	files { 
		"externals/libchdr/src/*.c",
	}
	defines {
		"FLAC__NO_DLL",
	}
	links {
		"miniz",
		"lzma",
		"flac",
	}

project "common"
	uuid "176665c5-37ff-4a42-bef8-02edaeb1b426"
	kind "StaticLib"
	location "build/libs/common"

	includedirs { 
		"src", 
		"externals/glm",
		"externals/json/include",
		"externals/miniz",
		"externals/libchdr/src",
	}

	files { 
		"src/**.h", 
		"src/**.cpp"
	}

	removefiles {
		"src/imgui/**.*",
		"src/renderer/**.*",
		"src/platform/**.*"
	}

	links {
		"miniz",
		"lzma",
		"flac",
		"chdr",
	}

	filter "system:windows" 
		defines "WIN32" 

	filter {"action:gmake", "system:windows"}
		defines {"_CRT_SECURE_NO_WARNINGS"}
		
	if _ACTION ~= nil then
		generateVersionFile()
	end

project "avocado"
	uuid "c2c4899a-ddca-491f-9a66-1d33173a553e"
	kind "ConsoleApp"
	location "build/libs/avocado"
	debugdir "."

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/glad/include",
		"externals/SDL2/include",
		"externals/glm",
		"externals/json/include",
		"externals/stb",
		"externals/libchdr/src",
	}

	links {
		"common",
		"miniz",
		"chdr",
		"lzma",
		"flac",
	}

	filter "system:windows" 
		defines "WIN32" 
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*"
		}
		links { 
			"SDL2",
			"glad",
			"imgui",
			"OpenGL32"
		}
		filter "platforms:x86"
			libdirs "externals/SDL2/lib/x86"
		
		filter "platforms:x64"
			libdirs "externals/SDL2/lib/x64"

	filter {"system:linux", "options:headless"}
		files { 
			"src/platform/headless/**.cpp",
			"src/platform/headless/**.h"
		}

-- TODO: Make headless and normal configurations
	filter {"system:linux", "not options:headless"}
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*"
		}
		links { 
			"stdc++fs", -- for experimental/filesystem
			"glad",
			"imgui",
		}
		buildoptions {getOutput("sdl2-config --cflags")}
		linkoptions {getOutput("sdl2-config --libs")}
		

	filter "system:macosx"
	    kind "WindowedApp"
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*"
		}
		links { 
			"glad",
			"imgui",
		}
		buildoptions {getOutput("sdl2-config --cflags")}
		linkoptions {getOutput("sdl2-config --libs")}

project "avocado_test"
	uuid "07e62c76-7617-4add-bfb5-a5dba4ef41ce"
	kind "ConsoleApp"
	location "build/libs/avocado_test"
	debugdir "."

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/glad/include",
		"externals/SDL2/include",
		"externals/glm",
		"externals/json/src",
		"externals/catch/single_include"
	}

	files { 
		"src/platform/null/**.*",
		"tests/unit/**.h",
		"tests/unit/**.cpp"
	}

	links {
		"common"
	}

project "avocado_autotest"
	uuid "fcc880bc-c6fe-4b2b-80dc-d247345a1274"
	kind "ConsoleApp"
	location "build/libs/avocado_autotest"
	debugdir "."

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/glad/include",
		"externals/SDL2/include",
		"externals/glm",
		"externals/json/src"
	}

	files { 
		"src/platform/null/**.*",
		"tests/auto/**.h",
		"tests/auto/**.cpp"
	}

	links {
		"common"
	}
