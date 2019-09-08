group "externals"
project "fmt"
    uuid "ffc1977c-c7bc-4d62-b7c7-6fa0b55c3625"
    kind "StaticLib"
    language "c++"
    location "../build/libs/fmt"
    includedirs { 
        "../externals/fmt/include"
    }
    files { 
        "../externals/fmt/src/*.cc",
    }