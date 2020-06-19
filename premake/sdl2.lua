-- Used for Windows build only

filter "system:windows" 
group "externals"
project "sdl2"
	uuid "C9CA7048-0F5F-4BE5-BF21-DDEE5F9A2172"
	kind "StaticLib"
	language "c"
    location "../build/libs/sdl"
    defines {
        "NDEBUG"
    }
	includedirs { 
		"../externals/SDL2/src",
		"../externals/SDL2/include",
	}
	files { 
        "../externals/SDL2/src/atomic/SDL_atomic.c",
        "../externals/SDL2/src/atomic/SDL_spinlock.c",
        "../externals/SDL2/src/audio/directsound/SDL_directsound.c",
        "../externals/SDL2/src/audio/disk/SDL_diskaudio.c",
        "../externals/SDL2/src/audio/dummy/SDL_dummyaudio.c",
        "../externals/SDL2/src/audio/SDL_audio.c",
        "../externals/SDL2/src/audio/SDL_audiocvt.c",
        "../externals/SDL2/src/audio/SDL_audiodev.c",
        "../externals/SDL2/src/audio/SDL_audiotypecvt.c",
        "../externals/SDL2/src/audio/SDL_mixer.c",
        "../externals/SDL2/src/audio/SDL_wave.c",
        "../externals/SDL2/src/audio/winmm/SDL_winmm.c",
        "../externals/SDL2/src/audio/wasapi/SDL_wasapi.c",
        "../externals/SDL2/src/audio/wasapi/SDL_wasapi_win32.c",
        "../externals/SDL2/src/core/windows/SDL_windows.c",
        "../externals/SDL2/src/core/windows/SDL_xinput.c",
        "../externals/SDL2/src/cpuinfo/SDL_cpuinfo.c",
        "../externals/SDL2/src/dynapi/SDL_dynapi.c",
        "../externals/SDL2/src/events/SDL_clipboardevents.c",
        "../externals/SDL2/src/events/SDL_displayevents.c",
        "../externals/SDL2/src/events/SDL_dropevents.c",
        "../externals/SDL2/src/events/SDL_events.c",
        "../externals/SDL2/src/events/SDL_gesture.c",
        "../externals/SDL2/src/events/SDL_keyboard.c",
        "../externals/SDL2/src/events/SDL_mouse.c",
        "../externals/SDL2/src/events/SDL_quit.c",
        "../externals/SDL2/src/events/SDL_touch.c",
        "../externals/SDL2/src/events/SDL_windowevents.c",
        "../externals/SDL2/src/file/SDL_rwops.c",
        "../externals/SDL2/src/filesystem/windows/SDL_sysfilesystem.c",
        "../externals/SDL2/src/haptic/SDL_haptic.c",
        "../externals/SDL2/src/haptic/windows/SDL_dinputhaptic.c",
        "../externals/SDL2/src/haptic/windows/SDL_windowshaptic.c",
        "../externals/SDL2/src/haptic/windows/SDL_xinputhaptic.c",
        "../externals/SDL2/src/hidapi/windows/hid.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapijoystick.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_gamecube.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_ps4.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_rumble.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_switch.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_xbox360.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_xbox360w.c",
        "../externals/SDL2/src/joystick/hidapi/SDL_hidapi_xboxone.c",
        "../externals/SDL2/src/joystick/SDL_gamecontroller.c",
        "../externals/SDL2/src/joystick/SDL_joystick.c",
        "../externals/SDL2/src/joystick/windows/SDL_dinputjoystick.c",
        "../externals/SDL2/src/joystick/windows/SDL_mmjoystick.c",
        "../externals/SDL2/src/joystick/windows/SDL_windowsjoystick.c",
        "../externals/SDL2/src/joystick/windows/SDL_xinputjoystick.c",
        "../externals/SDL2/src/libm/e_atan2.c",
        "../externals/SDL2/src/libm/e_exp.c",
        "../externals/SDL2/src/libm/e_fmod.c",
        "../externals/SDL2/src/libm/e_log.c",
        "../externals/SDL2/src/libm/e_log10.c",
        "../externals/SDL2/src/libm/e_pow.c",
        "../externals/SDL2/src/libm/e_rem_pio2.c",
        "../externals/SDL2/src/libm/e_sqrt.c",
        "../externals/SDL2/src/libm/k_cos.c",
        "../externals/SDL2/src/libm/k_rem_pio2.c",
        "../externals/SDL2/src/libm/k_sin.c",
        "../externals/SDL2/src/libm/k_tan.c",
        "../externals/SDL2/src/libm/s_atan.c",
        "../externals/SDL2/src/libm/s_copysign.c",
        "../externals/SDL2/src/libm/s_cos.c",
        "../externals/SDL2/src/libm/s_fabs.c",
        "../externals/SDL2/src/libm/s_floor.c",
        "../externals/SDL2/src/libm/s_scalbn.c",
        "../externals/SDL2/src/libm/s_sin.c",
        "../externals/SDL2/src/libm/s_tan.c",
        "../externals/SDL2/src/loadso/windows/SDL_sysloadso.c",
        "../externals/SDL2/src/power/SDL_power.c",
        "../externals/SDL2/src/power/windows/SDL_syspower.c",
        "../externals/SDL2/src/render/direct3d11/SDL_shaders_d3d11.c",
        "../externals/SDL2/src/render/direct3d/SDL_render_d3d.c",
        "../externals/SDL2/src/render/direct3d11/SDL_render_d3d11.c",
        "../externals/SDL2/src/render/direct3d/SDL_shaders_d3d.c",
        "../externals/SDL2/src/render/opengl/SDL_render_gl.c",
        "../externals/SDL2/src/render/opengl/SDL_shaders_gl.c",
        "../externals/SDL2/src/render/opengles2/SDL_render_gles2.c",
        "../externals/SDL2/src/render/opengles2/SDL_shaders_gles2.c",
        "../externals/SDL2/src/render/SDL_d3dmath.c",
        "../externals/SDL2/src/render/SDL_render.c",
        "../externals/SDL2/src/render/SDL_yuv_sw.c",
        "../externals/SDL2/src/render/software/SDL_blendfillrect.c",
        "../externals/SDL2/src/render/software/SDL_blendline.c",
        "../externals/SDL2/src/render/software/SDL_blendpoint.c",
        "../externals/SDL2/src/render/software/SDL_drawline.c",
        "../externals/SDL2/src/render/software/SDL_drawpoint.c",
        "../externals/SDL2/src/render/software/SDL_render_sw.c",
        "../externals/SDL2/src/render/software/SDL_rotate.c",
        "../externals/SDL2/src/SDL.c",
        "../externals/SDL2/src/SDL_assert.c",
        "../externals/SDL2/src/SDL_dataqueue.c",
        "../externals/SDL2/src/SDL_error.c",
        "../externals/SDL2/src/SDL_hints.c",
        "../externals/SDL2/src/SDL_log.c",
        "../externals/SDL2/src/sensor/dummy/SDL_dummysensor.c",
        "../externals/SDL2/src/sensor/SDL_sensor.c",
        "../externals/SDL2/src/stdlib/SDL_getenv.c",
        "../externals/SDL2/src/stdlib/SDL_iconv.c",
        "../externals/SDL2/src/stdlib/SDL_malloc.c",
        "../externals/SDL2/src/stdlib/SDL_qsort.c",
        "../externals/SDL2/src/stdlib/SDL_stdlib.c",
        "../externals/SDL2/src/stdlib/SDL_string.c",
        "../externals/SDL2/src/stdlib/SDL_strtokr.c",
        "../externals/SDL2/src/thread/generic/SDL_syscond.c",
        "../externals/SDL2/src/thread/SDL_thread.c",
        "../externals/SDL2/src/thread/windows/SDL_sysmutex.c",
        "../externals/SDL2/src/thread/windows/SDL_syssem.c",
        "../externals/SDL2/src/thread/windows/SDL_systhread.c",
        "../externals/SDL2/src/thread/windows/SDL_systls.c",
        "../externals/SDL2/src/timer/SDL_timer.c",
        "../externals/SDL2/src/timer/windows/SDL_systimer.c",
        "../externals/SDL2/src/video/dummy/SDL_nullevents.c",
        "../externals/SDL2/src/video/dummy/SDL_nullframebuffer.c",
        "../externals/SDL2/src/video/dummy/SDL_nullvideo.c",
        "../externals/SDL2/src/video/SDL_blit.c",
        "../externals/SDL2/src/video/SDL_blit_0.c",
        "../externals/SDL2/src/video/SDL_blit_1.c",
        "../externals/SDL2/src/video/SDL_blit_A.c",
        "../externals/SDL2/src/video/SDL_blit_auto.c",
        "../externals/SDL2/src/video/SDL_blit_copy.c",
        "../externals/SDL2/src/video/SDL_blit_N.c",
        "../externals/SDL2/src/video/SDL_blit_slow.c",
        "../externals/SDL2/src/video/SDL_bmp.c",
        "../externals/SDL2/src/video/SDL_clipboard.c",
        "../externals/SDL2/src/video/SDL_egl.c",
        "../externals/SDL2/src/video/SDL_fillrect.c",
        "../externals/SDL2/src/video/SDL_pixels.c",
        "../externals/SDL2/src/video/SDL_rect.c",
        "../externals/SDL2/src/video/SDL_RLEaccel.c",
        "../externals/SDL2/src/video/SDL_shape.c",
        "../externals/SDL2/src/video/SDL_stretch.c",
        "../externals/SDL2/src/video/SDL_surface.c",
        "../externals/SDL2/src/video/SDL_video.c",
        "../externals/SDL2/src/video/SDL_vulkan_utils.c",
        "../externals/SDL2/src/video/SDL_yuv.c",
        "../externals/SDL2/src/video/windows/SDL_windowsclipboard.c",
        "../externals/SDL2/src/video/windows/SDL_windowsevents.c",
        "../externals/SDL2/src/video/windows/SDL_windowsframebuffer.c",
        "../externals/SDL2/src/video/windows/SDL_windowskeyboard.c",
        "../externals/SDL2/src/video/windows/SDL_windowsmessagebox.c",
        "../externals/SDL2/src/video/windows/SDL_windowsmodes.c",
        "../externals/SDL2/src/video/windows/SDL_windowsmouse.c",
        "../externals/SDL2/src/video/windows/SDL_windowsopengl.c",
        "../externals/SDL2/src/video/windows/SDL_windowsopengles.c",
        "../externals/SDL2/src/video/windows/SDL_windowsshape.c",
        "../externals/SDL2/src/video/windows/SDL_windowsvideo.c",
        "../externals/SDL2/src/video/windows/SDL_windowsvulkan.c",
        "../externals/SDL2/src/video/windows/SDL_windowswindow.c",
        "../externals/SDL2/src/video/yuv2rgb/yuv_rgb.c",
    }
    links {
        "setupapi",
        "winmm",
        "imm32",
        "version",
    }
    defines {
        "SDL_DISABLE_WINDOWS_IME",
        "HAVE_LIBC",
    }