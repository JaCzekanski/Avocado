workspace "Avocado"
	configurations { "Debug", "Release" }
	
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
			"src/renderer/opengl/**.cpp",
			"src/renderer/opengl/**.h",
			"src/platform/windows/**.cpp",
			"src/platform/windows/**.h"
		}
		links { 
			"SDL2",
			"glad",
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
			"-Wno-write-strings",
			"-Wno-unused-private-field",
			"-Wno-unused-const-variable",
			"-fno-operator-names",
			"-fno-exceptions"
		}
