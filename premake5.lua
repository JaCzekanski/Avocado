include "premake/tools.lua"

workspace "Avocado"    
	configurations { "debug", "release" }
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


filter "kind:*App"
	targetdir "build/%{cfg.buildcfg}_%{cfg.platform}"

filter {"kind:*App", "platforms:x86"}
	targetdir "build/%{cfg.buildcfg}"


filter "configurations:Debug"
	defines { "DEBUG" }
	symbols "On"

filter "configurations:Release"
	staticruntime "on"
	defines { "NDEBUG" }
	flags { "MultiProcessorCompile" }
	optimize "Full"

filter {"configurations:Release", "system:windows or system:macosx"}
	if os.getenv("CI") == true then
		flags { "LinkTimeOptimization" }
	end

filter "action:vs*"
	defines "_CRT_SECURE_NO_WARNINGS"

filter "action:gmake"
	buildoptions { 
		"-Wall",
		"-Wextra",
	}

include "premake/chdr.lua"
include "premake/flac.lua"
include "premake/fmt.lua"
include "premake/glad.lua"
include "premake/imgui.lua"
include "premake/lzma.lua"
include "premake/miniz.lua"

project "core"
	uuid "176665c5-37ff-4a42-bef8-02edaeb1b426"
	kind "StaticLib"
	location "build/libs/core"

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
		"externals/glm",
		"externals/json/include",
		"externals/stb",
		"externals/libchdr/src",
		"externals/fmt/include",
	}

	links {
		"core",
		"miniz",
		"chdr",
		"lzma",
		"flac",
		"fmt",
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
			"SDL2",
			"glad",
			"imgui",
			"OpenGL32"
		}

	filter {"system:windows", "not options:headless", "platforms:x86"}
		libdirs "externals/SDL2/lib/x86"
		
	filter {"system:windows", "not options:headless", "platforms:x64"}
		libdirs "externals/SDL2/lib/x64"


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
		

	filter {"system:macosx", "not options:headless"}
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
		"core"
	}
