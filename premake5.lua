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
			"/usr/include/SDL2"
		}
	libdirs { os.findlib("SDL2") }
	links { 
			"SDL2"
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
        links { "OpenGL32" }
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