group "externals"
project "flac"
    uuid "2f208b4f-2e69-408e-b0df-b0e72b031b02"
    kind "StaticLib"
    language "c"
    location "../build/libs/flac"
    includedirs { 
        "../externals/flac/src/libFLAC/include",
        "../externals/flac/include",
    }
    files { 
        "../externals/flac/src/libFLAC/bitmath.c",
        "../externals/flac/src/libFLAC/bitreader.c",
        "../externals/flac/src/libFLAC/cpu.c",
        "../externals/flac/src/libFLAC/crc.c",
        "../externals/flac/src/libFLAC/fixed.c",
        "../externals/flac/src/libFLAC/fixed_intrin_sse2.c",
        "../externals/flac/src/libFLAC/fixed_intrin_ssse3.c",
        "../externals/flac/src/libFLAC/float.c",
        "../externals/flac/src/libFLAC/format.c",
        "../externals/flac/src/libFLAC/lpc.c",
        "../externals/flac/src/libFLAC/lpc_intrin_avx2.c",
        "../externals/flac/src/libFLAC/lpc_intrin_sse2.c",
        "../externals/flac/src/libFLAC/lpc_intrin_sse41.c",
        "../externals/flac/src/libFLAC/lpc_intrin_sse.c",
        "../externals/flac/src/libFLAC/md5.c",
        "../externals/flac/src/libFLAC/memory.c",
        "../externals/flac/src/libFLAC/metadata_iterators.c",
        "../externals/flac/src/libFLAC/metadata_object.c",
        "../externals/flac/src/libFLAC/stream_decoder.c",
        "../externals/flac/src/libFLAC/window.c",
    }
    filter "system:windows" 
        files {
            "../externals/flac/src/libFLAC/windows_unicode_filenames.c",
        }
    filter {}
    defines {
        "PACKAGE_VERSION=\"1.3.2\"",
        "FLAC__HAS_OGG=0", 
        "FLAC__NO_DLL", 
        "HAVE_LROUND", 
        "HAVE_STDINT_H", 
        "HAVE_STDLIB_H",
    }
    
    filter "system:windows" 
        disablewarnings { 
            "4018", -- 'expression' : signed/unsigned mismatch
            "4244", -- 'conversion' conversion from 'type1' to 'type2', possible loss of data
            "4267", -- 'var' : conversion from 'size_t' to 'type', possible loss of data
            "4334", -- 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
            "4996", -- The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: new_name.
        }
        
    filter "system:linux or system:macosx or system:android"
        disablewarnings {
            "unused-parameter",
            "unused-function",
        }