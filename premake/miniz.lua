group "externals"
project "miniz"
	uuid "4D28CBE8-3092-4400-B67B-FD51FCAFBD34"
	kind "StaticLib"
	language "c"
	location "../build/libs/miniz"
	includedirs { 
		"../externals/miniz"
	}
	files { 
		"../externals/miniz/*.c",
	}