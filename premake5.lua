workspace "Avocado"
	configurations { "Debug", "Release" }

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

project "glad"
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

project "Avocado"
	kind "ConsoleApp"
	language "c++"
	location "build/libs/Avocado"
	targetdir "build/%{cfg.buildcfg}"
	debugdir "."
	flags { "C++14" }

	includedirs { 
		"src", 
		"externals/imgui",
		"externals/SDL2/include",
		"externals/glad/include",
		"externals/glm"
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
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		
	configuration { "windows" }
		defines { "WIN32" }
		libdirs { os.findlib("SDL2") }
		libdirs { 
			"externals/SDL2/lib/x86"
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
		defines {"_CRT_SECURE_NO_WARNINGS"}


	configuration { "linux", "gmake" }
		files { 
			"src/platform/headless/**.cpp",
			"src/platform/headless/**.h"
		}
		buildoptions { 
			"-Wall",
			"-Wextra",
			"-Wno-unused-parameter",
		}
