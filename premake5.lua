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
		"-Wno-macro-redefined",
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

project "common"
	uuid "176665c5-37ff-4a42-bef8-02edaeb1b426"
	kind "StaticLib"
	location "build/libs/common"

	dependson {
		"miniz"
	}

	includedirs { 
		"src", 
		"externals/glm",
		"externals/json/include",
		"externals/miniz"
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
		"miniz"
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
	dependson { "common" }

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/glad/include",
		"externals/SDL2/include",
		"externals/glm",
		"externals/json/include",
		"externals/stb",
	}

	links {
		"common",
		"miniz"
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
			"glad",
			"imgui",
		}
		buildoptions {"`sdl2-config --cflags`"}
		linkoptions {"`sdl2-config --libs`"}
		

	filter "system:macosx"
		files { 
			"src/imgui/**.*",
			"src/renderer/opengl/**.*",
			"src/platform/windows/**.*"
		}
		links { 
			"glad",
			"imgui",
		}
		buildoptions {"`sdl2-config --cflags`"}
		linkoptions {"`sdl2-config --libs`"}

project "avocado_test"
	uuid "07e62c76-7617-4add-bfb5-a5dba4ef41ce"
	kind "ConsoleApp"
	location "build/libs/avocado_test"
	debugdir "."
	dependson { "common" }

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
	dependson { "common" }

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
