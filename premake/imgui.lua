group "externals"
project "imgui"
	uuid "a8f18b69-f15a-4804-80f7-e8f80ab91369"
	kind "StaticLib"
	location "../build/libs/imgui"
	includedirs { 
		"../externals/imgui",
		"../externals/glad/include",
	}
	files { 
		"../externals/imgui/*.cpp",
	}