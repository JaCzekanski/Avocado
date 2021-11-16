include "premake/tools.lua"
require "./premake/androidmk"

-- Windows support 32bit and 64bit versions
-- Linux - as well
-- MacOS is 64bit only
-- Android has ARM architecture - Android studio decides on ABI
-- Web - single architecture

workspace "Avocado"    
	configurations { "debug", "release" }
	startproject "avocado"
	
ndkstl "c++_static"
ndkplatform "android-24"

newoption {
	trigger = "enable-io-log",
	description = "Enable IO access log",
}
filter "options:enable-io-log"
	defines "ENABLE_IO_LOG"

newoption {
	trigger = "asan",
	description = "Build with Address Sanitizer enabled"
}
filter "options:asan"
	symbols "On"
	buildoptions {"-fsanitize=address"}
	linkoptions {"-fsanitize=address"}
	
newoption {
	trigger = "msan",
	description = "Build with Memory Sanitizer enabled"
}
filter "options:msan"
	symbols "On"
	buildoptions {"-fsanitize=memory"}
	linkoptions {"-fsanitize=memory"}
	
newoption {
	trigger = "ubsan",
	description = "Build with Undefined Behaviour Sanitizer enabled"
}
filter "options:ubsan"
	symbols "On"
	buildoptions {"-fsanitize=undefined"}
	linkoptions {"-fsanitize=undefined"}

newoption {
	trigger = "time-trace",
	description = "Build with -ftime-trace (clang only)"
}
filter "options:time-trace"
	buildoptions {"-ftime-trace"}
	linkoptions {"-ftime-trace"}


filter {}
	language "c++"
	cppdialect "C++17"
	defines { 'BUILD_ARCH="%{cfg.system}"' }
	flags { "MultiProcessorCompile" }
	exceptionhandling "On"
	rtti "On"

filter "system:windows"
	platforms {"x86", "x64"}
	defaultplatform "x64"

filter "system:linux"
	platforms {"x86", "x64"}
	defaultplatform "x64"

filter "system:macosx"
	platforms {"x64"}
	defaultplatform "x64"
	xcodebuildsettings {
		["ARCHS"] = "x86_64",
		["ALWAYS_SEARCH_USER_PATHS"] = "YES",
		["MACOSX_DEPLOYMENT_TARGET"] = "10.12",
	}
	-- arm64 builds are currently broken (SDL2 from brew linking errors)
	-- HACK: Temporary SDL2.0.16 xcode build fix
    -- defines { "SDL_DISABLE_IMMINTRIN_H" }

filter "system:android"
	platforms {"arm"}
	defines { "USE_OPENGLES"}
	
filter "platforms:x86"
	architecture "x32"
	vectorextensions "SSE3"

filter "platforms:x64"
	architecture "x64"
	vectorextensions "SSE3"

filter "platforms:arm"
	architecture "arm"
	vectorextensions "NEON"

filter "action:gmake"
	buildoptions { 
		"-Wall",
		"-Wextra",
	}

filter "action:vs*"
	defines "_CRT_SECURE_NO_WARNINGS"

filter { "action:vs*", "configurations:Debug" }
	defines "_ITERATOR_DEBUG_LEVEL=0"
	
filter "action:xcode*"
	buildoptions "-fvisibility=hidden"
	linkoptions "-fvisibility=hidden"
	
filter "kind:*App"
	targetdir "build/%{cfg.buildcfg}_%{cfg.platform}"

filter {"kind:*App", "platforms:x86"}
	targetdir "build/%{cfg.buildcfg}"
	
filter {"kind:*App", "system:android"}
	targetdir "build/%{cfg.buildcfg}"

filter "configurations:Debug"
	defines { "DEBUG" }
	symbols "On"
	optimize "Debug"

filter "configurations:Release"
	staticruntime "on"
	defines { "NDEBUG" }
	optimize "Full"

-- LTO on Linux fails with "error adding symbols: Archive has no index; run ranlib to add one"
filter {"configurations:Release", "not system:linux"}
	if os.getenv("CI") ~= nil then
		flags { "LinkTimeOptimization" }
	end

filter {"configurations:Release", "system:linux"}
	if os.getenv("CI") ~= nil then
		linkoptions { "-static-libstdc++" }
	end

include "premake/chdr.lua"
include "premake/flac.lua"
include "premake/glad.lua"
include "premake/imgui.lua"
include "premake/lzma.lua"
include "premake/miniz.lua"
include "premake/fmt.lua"
include "premake/stb.lua"

if os.istarget("windows") then
	include "premake/sdl2.lua"
end

group "avocado"
project "core"
	uuid "176665c5-37ff-4a42-bef8-02edaeb1b426"
	kind "StaticLib"
	location "build/libs/common"

	includedirs { 
		"src", 
		"externals/json/include",
		"externals/stb",
		"externals/miniz",
		"externals/libchdr/src",
		"externals/EventBus/lib/include",
		"externals/magic_enum/include",
		"externals/fmt/include",
		"externals/cereal/include",
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
		"fmt",
		"stb",
	}

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
		"externals/json/include",
		"externals/stb",
		"externals/libchdr/src",
		"externals/filesystem/include",
		"externals/EventBus/lib/include",
		"externals/magic_enum/include",
		"externals/fmt/include",
		"externals/cereal/include",
	}

	links {
		"core",
		"miniz",
		"chdr",
		"lzma",
		"flac",
		"fmt",
		"stb",
	}

	filter "options:headless"
		files { 
			"src/platform/headless/**.cpp",
			"src/platform/headless/**.h"
		}

	filter {"system:windows", "not options:headless"}
		includedirs { 
			"externals/SDL2/include",
		} 
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*"
		}
		links { 
			"sdl2",
			"glad",
			"imgui",
			"OpenGL32"
		}

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
			"pthread",
		}
		buildoptions {getOutput("sdl2-config --cflags")}
		linkoptions {getOutput("sdl2-config --libs")}
		

	filter {"system:macosx"}
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
		linkoptions {getOutput("sdl2-config --static-libs")}
	
	filter {"system:android"}
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*",
		}
		links {
			"glad",
			"imgui",
			"GLESv1_CM",
			"GLESv2",
			"GLESv3",
			"log",
		}

		amk_includes {
			"externals/SDL2/Android.mk",
		}

		amk_sharedlinks {
			"SDL2",
		}

group "tests"
project "avocado_test"
	uuid "07e62c76-7617-4add-bfb5-a5dba4ef41ce"
	kind "ConsoleApp"
	location "build/libs/avocado_test"
	debugdir "."

	includedirs { 
		"src", 
		"externals/catch/single_include"
	}

	files { 
		"src/platform/null/**.*",
		"tests/unit/**.h",
		"tests/unit/**.cpp"
	}

	links {
		"core"
	}

project "avocado_autotest"
	uuid "fcc880bc-c6fe-4b2b-80dc-d247345a1274"
	kind "ConsoleApp"
	location "build/libs/avocado_autotest"
	debugdir "."

	includedirs { 
		"src", 
	}

	files { 
		"src/platform/null/**.*",
		"tests/auto/**.h",
		"tests/auto/**.cpp"
	}

	links {
		"core",
		"fmt"
	}
