import os


def is_active():
    return True


def get_name():
    return "Switch2"


def can_build():
    devkitpro = os.environ.get("DEVKITPRO", "/opt/devkitpro")
    required = [
        "devkitA64/bin/aarch64-none-elf-gcc",
        "libounce/include/switch.h",
        "libounce/switch.specs",
        "portlibs/switch2/include/EGL/egl.h",
        "tools/bin/elf2nro",
    ]
    for path in required:
        if not os.path.exists(os.path.join(devkitpro, path)):
            print("Switch 2 dependency missing: " + path)
            return False
    return True


def get_opts():
    from SCons.Variables import BoolVariable, EnumVariable

    return [
        EnumVariable("debug_symbols", "Add debugging symbols", "yes", ("yes", "no", "full")),
        BoolVariable("separate_debug_symbols", "Create a separate debug symbol file", False),
    ]


def get_flags():
    return [
        ("tools", False),
        ("builtin_bullet", True),
        ("builtin_enet", True),
        ("builtin_freetype", True),
        ("builtin_libogg", True),
        ("builtin_libpng", True),
        ("builtin_libtheora", True),
        ("builtin_libvorbis", True),
        ("builtin_libvpx", True),
        ("builtin_libwebp", True),
        ("builtin_wslay", True),
        ("builtin_mbedtls", True),
        ("builtin_miniupnpc", True),
        ("builtin_opus", True),
        ("builtin_pcre2", True),
        ("builtin_pcre2_with_jit", False),
        ("builtin_squish", True),
        ("builtin_zlib", True),
        ("builtin_zstd", True),
    ]


def configure(env):
    devkitpro = os.environ.get("DEVKITPRO", "/opt/devkitpro")
    env["CC"] = "aarch64-none-elf-gcc"
    env["CXX"] = "aarch64-none-elf-g++"
    env["LD"] = "aarch64-none-elf-g++"
    env["AR"] = "aarch64-none-elf-ar"
    env["RANLIB"] = "aarch64-none-elf-ranlib"
    env["ENV"]["DEVKITPRO"] = devkitpro

    tool_paths = [
        devkitpro + "/portlibs/switch2/bin",
        devkitpro + "/devkitA64/bin",
        devkitpro + "/tools/bin",
    ]
    env["ENV"]["PATH"] = ":".join(tool_paths) + ":" + os.environ["PATH"]
    os.environ["PATH"] = env["ENV"]["PATH"]

    arch = ["-mcpu=cortex-a78c+crypto", "-mtp=soft", "-fPIE"]
    env.Prepend(CCFLAGS=arch + ["-ffunction-sections", "-fdata-sections", "-pipe"])
    env.Prepend(LINKFLAGS=arch + [
        "-specs=" + devkitpro + "/libounce/switch.specs",
        "-Wl,--gc-sections",
        "-pipe",
    ])

    env.Prepend(CPPPATH=[
        devkitpro + "/portlibs/switch2/include",
        devkitpro + "/libounce/include",
    ])
    env.Append(CPPPATH=["#platform/switch2", "#drivers/audren"])
    env.Append(CPPFLAGS=[
        "-D__SWITCH__",
        "-D__SWITCH2__",
        "-DHOMEBREW_ENABLED",
        "-DHORIZON_ENABLED",
        "-DLIBC_FILEIO_ENABLED",
        "-DUNIX_SOCKET_UNAVAILABLE",
        "-DOPENGL_ENABLED",
        "-DGLES_ENABLED",
        "-DPTHREAD_ENABLED",
        "-DPTHREAD_NO_RENAME",
        "-DPOSH_COMPILER_GCC",
        "-DPOSH_OS_HORIZON",
        '-DPOSH_OS_STRING="horizon"',
    ])
    env.Append(LIBPATH=[
        devkitpro + "/portlibs/switch2/lib",
        devkitpro + "/libounce/lib",
    ])

    if env["target"] == "release":
        env.Prepend(CCFLAGS=["-O2"] if env["optimize"] == "speed" else ["-Os"])
    elif env["target"] == "release_debug":
        env.Prepend(CCFLAGS=["-O2", "-DDEBUG_ENABLED"] if env["optimize"] == "speed" else ["-Os", "-DDEBUG_ENABLED"])
    else:
        env.Prepend(CCFLAGS=["-O0", "-g3", "-DDEBUG_ENABLED", "-DDEBUG_MEMORY_ENABLED"])

    if env["debug_symbols"] == "yes":
        env.Prepend(CCFLAGS=["-g1"])
    elif env["debug_symbols"] == "full":
        env.Prepend(CCFLAGS=["-g2"])

    env["bits"] = "64"
    env.Append(LIBS=["EGL", "GLESv2", "glapi", "drm_nouveau", "nx"])
