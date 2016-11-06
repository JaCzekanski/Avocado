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
		"externals/SDL2/include"
	}
	files { 
		"externals/SDL_net/SDLnet*.c"
	}

	configuration { "linux", "gmake" }
		buildoptions { 
			"`pkg-config --cflags sdl2`"
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
	location "build/libs/Avocado"
	targetdir "build/%{cfg.buildcfg}"
	flags { "C++14" }

	includedirs { 
		".", 
		"src", 
		"externals/imgui",
		"externals/SDL_net",
		"externals/glad/include",
		"externals/glm"
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

	configuration "headless"
		defines { "HEADLESS" }

	configuration "not headless"
		libdirs { os.findlib("SDL2") }
		links { 
				"SDL2",
				"SDL_net",
				"glad"
		}

	configuration { "windows" }
		defines { "WIN32" }
		links { 
			"OpenGL32",
			"ws2_32",
			"Iphlpapi"
		}
		defines {"_CRT_SECURE_NO_WARNINGS"}

	configuration { "linux", "gmake" }
		buildoptions { 
			"`pkg-config --cflags sdl2`"
		}
		linkoptions { 
			"`pkg-config --libs sdl2`"
		}
		links { "GL" }
		buildoptions { 
			"-Wall",
			"-Wno-write-strings",
			"-Wno-unused-private-field",
			"-Wno-unused-const-variable",
			"-fno-operator-names",
			"-fno-exceptions"
		}
