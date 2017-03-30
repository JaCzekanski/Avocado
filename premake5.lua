workspace "Avocado"
	configurations { "Debug", "Release" }

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

	configuration { "windows" }
		defines { "WIN32" }

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
	debugdir "."
	flags { "C++14" }

	includedirs { 
		".", 
		"src", 
		"externals/imgui",
		"externals/SDL_net",
		"externals/SDL2/include",
		"externals/glad/include",
		"externals/glm"
	}

	files { 
		"src/**.h", 
		"src/**.cpp"
	}
	removefiles {
		"src/renderer/**.*",
		"src/platform/**.*",
	}
	
	links { 
		"SDL2",
		"SDL_net",
		"glad"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
	
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
			"src/renderer/opengl/**.cpp",
			"src/renderer/opengl/**.h"
			"src/platform/windows/**.cpp",
			"src/platform/windows/**.h"
		}
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
		files { 
			"src/platform/headless/**.cpp",
			"src/platform/headless/**.h"
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
