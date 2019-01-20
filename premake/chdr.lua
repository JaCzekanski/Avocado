group "externals"
project "chdr"
    uuid "d148de3d-efbb-4f1d-88bc-a112be68fc04"
    kind "StaticLib"
    language "c"
    location "../build/libs/chdr"
    includedirs { 
        "../externals/flac/include",
        "../externals/lzma/C",
        "../externals/libchdr/src",
        "../externals/miniz",
    }
    files { 
        "../externals/libchdr/src/*.c",
    }
    defines {
        "FLAC__NO_DLL",
    }
    links {
        "miniz",
        "lzma",
        "flac",
    }