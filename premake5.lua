workspace "Avocado"
	configurations { "Debug", "Release" }

project "Avocado"
	kind "ConsoleApp"
	language "c++"
	targetdir "build/%{cfg.buildcfg}"
	
	includedirs { 
			".", 
			"src", 
			"externals/imgui",
			"/usr/include/SDL2"
		}
	libdirs { os.findlib("SDL2") }
	links { 
			"SDL2",
			"GL"
	}

	files { 
		"src/**.h", 
		"src/**.cpp", 
		"externals/imgui/*.cpp",
		"externals/imgui/examples/sdl_opengl_example/imgui_impl_sdl.cpp"
	}
	buildoptions { 
			"-stdlib=libc++",
			"-std=c++11",
			"-Wno-write-strings",
			"-fno-operator-names",
			"-fno-exceptions"
	}

	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }	
	
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		
