group "externals"
project "stb"
	uuid "70748C53-02F2-42E7-AE0A-8CBD19918850"
	kind "StaticLib"
	language "c"
	location "../build/libs/stb"
	includedirs { 
		"../externals/stb"
	}
	files { 
		"../externals/stb/*.c",
	}