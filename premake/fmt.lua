group "externals"
project "fmt"
	uuid "B47FA23F-D2CE-4C58-A5F5-61A8E145437B"
	kind "StaticLib"
	language "c++"
	location "../build/libs/fmt"
	includedirs { 
		"../externals/fmt/include"
	}
	files { 
		"../externals/fmt/src/*.cc",
	}