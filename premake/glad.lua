group "externals"
project "glad"
    uuid "9add6bd2-2372-4614-a367-2e8863415083"
    kind "StaticLib"
    language "c"
    location "../build/libs/glad"
    includedirs { 
        "../externals/glad/include"
    }
    files { 
        "../externals/glad/src/*.c",
    }