import os
import sys

CPPPATH = []
LIBPATH = ["."]

if sys.platform == 'win32':
    CPPPATH += ["VS/windeps/SDL2/include"]
    LIBPATH += ["VS/windeps/SDL2/lib/x64"]
    env = Environment(MSVC_VERSION="14.0",
        CPPPATH=CPPPATH,
        CPPFLAGS="/GL /O2 /Oi /MD /EHsc /openmp",
        LINKFLAGS="/LTCG",
        )
else:
    envargs = {}
    if sys.platform == 'darwin':
        envargs['CC'] = 'gcc-4.9'
        envargs['CXX'] = 'g++-4.9'
    env = Environment(CPPPATH=CPPPATH,
        CPPFLAGS="-O3 -std=c++14 -fopenmp",
        LINKFLAGS="-pthread -fopenmp",
        **envargs
        )


srcglob = Glob("src/*.cpp")

libwsim_files = [f for f in srcglob if f.name not in ("main.cpp", "viewer.cpp")]

libwsim = env.StaticLibrary("libwsim", libwsim_files)
wsim = env.Program(target="wsim", source=["src/main.cpp"], LIBS="libwsim", LIBPATH=LIBPATH)

try:
    env.ParseConfig('sdl2-config --cflags')
    env.ParseConfig('sdl2-config --libs')
except OSError:
    pass

viewer = env.Program(target="wsim_viewer", source=["src/viewer.cpp"], LIBS=["libwsim", "SDL2"], LIBPATH=LIBPATH)
