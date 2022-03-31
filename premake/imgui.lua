group "externals"
project "imgui"
	uuid "a8f18b69-f15a-4804-80f7-e8f80ab91369"
	kind "StaticLib"
	location "../build/libs/imgui"
	defines {
		"IMGUI_DISABLE_WIN32_FUNCTIONS",
		"IMGUI_DISABLE_OSX_FUNCTIONS"
	}
	includedirs { 
		"../externals/imgui",
		"../externals/imgui/backends",
		"../externals/glad/include",
		"../externals/SDL2/include",
	}
	files { 
		"../externals/imgui/*.cpp",
		"../externals/imgui/backends/imgui_impl_opengl3.cpp",
		"../externals/imgui/backends/imgui_impl_sdl.cpp",
		"../externals/imgui/misc/cpp/*.cpp",
	}