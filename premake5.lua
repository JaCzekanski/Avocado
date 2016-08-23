workspace "Avocado"
	configurations { "Debug", "Release" }

project "Avocado"
	kind "ConsoleApp"
	language "c++"
	targetdir "build/%{cfg.buildcfg}"
	objdir "build/obj/%{cfg.buildcfg}"
	
	includedirs { 
			".", 
			"src", 
			"externals/imgui",
			"externals/SDL_net",
			"/usr/include/SDL2"
		}
	libdirs { os.findlib("SDL2") }
	links { 
			"SDL2"
	}

	files { 
		"externals/SDL_net/SDLnet*.c",
		"src/**.h", 
		"src/**.cpp"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
	
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"

    configuration { "windows" }
		defines { "WIN32" }
        links { 
        	"OpenGL32",
        	"ws2_32",
        	"Iphlpapi"
        }
		defines {"_CRT_SECURE_NO_WARNINGS"}

	configuration { "linux", "gmake" }
		toolset "clang"
        links { "GL" }
		buildoptions { 
			"-stdlib=libc++",
			"-std=c++14",
			"-Wno-write-strings",
			"-fno-operator-names",
			"-fno-exceptions",
			"-Wall"
		}