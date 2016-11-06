workspace "Avocado"
	configurations { "Debug", "Release" }

newoption {
		trigger = "headless",
		description = "Build without window creation"
}

project "SDL_net"
	kind "StaticLib"
	language "c"
	location "build/libs/SDL_net"
	includedirs { 
		"externals/SDL_net",
		"/usr/include/SDL2"
	}
	files { 
		"externals/SDL_net/SDLnet*.c"
	}

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
		"externals/glad/include",
		"/usr/include/SDL2"
	}
	libdirs { os.findlib("SDL2") }
	links { 
			"SDL2",
			"SDL_net",
			"glad"
	}

	files { 
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
	configuration "headless"
		defines { "HEADLESS" }
