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

	f = io.open('src/version.h', 'w')
	f:write('#pragma once\n')
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
	configurations { "Debug", "Release", "FastDebug" }

newoption {
	trigger = "disable-load-delay-slots",
	description = "Disable load delay slots (faster execution)",
}
filter "not options:disable-load-delay-slots"
	defines "ENABLE_LOAD_DELAY_SLOTS"

newoption {
	trigger = "enable-breakpoints",
	description = "Enable breakpoints (used in autotests)",
}
filter "options:enable-breakpoints"
	defines "ENABLE_BREAKPOINTS"

newoption {
	trigger = "enable-io-log",
	description = "Enable IO access log",
}
filter "options:enable-io-log"
	defines "ENABLE_IO_LOG"


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

filter "action:vs*"
    flags { "MultiProcessorCompile" }
	defines "_CRT_SECURE_NO_WARNINGS"

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

project "imgui"
	uuid "a8f18b69-f15a-4804-80f7-e8f80ab91369"
	kind "StaticLib"
	language "c++"
	location "build/libs/imgui"
	includedirs { 
		"externals/imgui",
		"externals/glad/include",
	}
	files { 
		"externals/imgui/imgui.cpp",
		"externals/imgui/imgui_draw.cpp",
	}

project "avocado"
	uuid "c2c4899a-ddca-491f-9a66-1d33173a553e"
	kind "ConsoleApp"
	language "c++"
	location "build/libs/avocado"
	targetdir "build/%{cfg.buildcfg}"
	debugdir "."
	flags { "C++14" }

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/glad/include",
		"externals/SDL2/include",
		"externals/glm",
		"externals/json/include"
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

	filter "action:vs*"
		libdirs {
			os.findlib("SDL2")
		}
		libdirs { 
			"externals/SDL2/lib/x86"
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

	filter {"action:gmake", "system:windows"}
		defines {"_CRT_SECURE_NO_WARNINGS"}
		libdirs { 
			"C:/sdk/SDL2-2.0.5/lib/x86"
		}
		includedirs {
			"C:/sdk/SDL2-2.0.5/include"
		}
		
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

	if _ACTION ~= nil then
		generateVersionFile()
	end
	

project "avocado_test"
	uuid "07e62c76-7617-4add-bfb5-a5dba4ef41ce"
	kind "ConsoleApp"
	language "c++"
	location "build/libs/avocado_test"
	targetdir "build/%{cfg.buildcfg}"
	debugdir "."
	flags { "C++14" }
	dependson { "avocado" }

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
		"tests/*.h",
		"tests/*.cpp"
	}

	links {
		"avocado"
	}
