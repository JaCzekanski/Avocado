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

project "avocado"
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
		"externals/json/src"
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

	filter "configurations:FastDebug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Speed"
        editandcontinue "On"

	filter "action:vs*"
        flags { "MultiProcessorCompile" }
		defines "_CRT_SECURE_NO_WARNINGS"
		libdirs {
			os.findlib("SDL2")
		}
		libdirs { 
			"externals/SDL2/lib/x86"
		}

	filter "action:gmake"
		buildoptions { 
			"-Wall",
			"-Wextra",
			"-Wno-unused-parameter",
			"-Wno-macro-redefined",
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
	
