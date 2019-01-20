group "externals"
project "lzma"
    uuid "af99f9c5-e14a-4478-bac5-07d457753d35"
    kind "StaticLib"
    language "c"
    location "../build/libs/lzma"
    includedirs { 
        "../externals/lzma/C",
    }
    files { 
        "../externals/lzma/C/Alloc.c",
        "../externals/lzma/C/Bra86.c",
        "../externals/lzma/C/Bra.c",
        "../externals/lzma/C/BraIA64.c",
        "../externals/lzma/C/CpuArch.c",
        "../externals/lzma/C/Delta.c",
        "../externals/lzma/C/LzFind.c",
        "../externals/lzma/C/Lzma86Dec.c",
        "../externals/lzma/C/Lzma86Enc.c",
        "../externals/lzma/C/LzmaDec.c",
        "../externals/lzma/C/LzmaEnc.c",
        "../externals/lzma/C/LzmaLib.c",
        "../externals/lzma/C/Sort.c",
    }
    defines {
        "_7ZIP_ST"
    }