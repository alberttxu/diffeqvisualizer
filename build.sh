#!/bin/sh

CC=c++
CFLAGS="-std=c++11 -g3"

WARNINGS="\
-Wall \
-Wextra \
-Wdouble-promotion \
-Wno-unused-parameter \
-Wno-unused-function \
-Wconversion \
-Wno-sign-conversion \
"

OS=`uname`
echo "Operating system: $OS"

if [ $OS = Linux ]; then
   JULIA_DIR="$HOME/julia-1.9.2"
   INCLUDES="\
   -Idependencies/raylib/src \
   -I$JULIA_DIR/include/julia \
   "
   LIBS="\
   dependencies/raylib/src/libraylib.a \
   -L$JULIA_DIR/lib
   -ljulia \
   -Wl,-rpath,$JULIA_DIR/lib \
   -l m \
   "

elif [ $OS = Darwin ]; then
   INCLUDES="\
   -I/opt/local/include \
   -I/opt/local/include/julia \
   -Idependencies/raylib/src \
   "
   LIBS="\
   -L/opt/local/lib \
   -ljulia \
   -Wl,-rpath,/opt/local/lib \
   dependencies/rlImGui/_bin/Debug/librlImGui.a \
   -lc++
   dependencies/cimgui/cimgui.o \
   dependencies/cimgui/imgui/imgui.o \
   dependencies/cimgui/imgui/imgui_draw.o \
   dependencies/cimgui/imgui/imgui_tables.o \
   dependencies/cimgui/imgui/imgui_widgets.o \
   dependencies/raylib/src/libraylib.a \
   -framework Cocoa -framework OpenGL -framework IOKit \
   "

   # -lraylib \

   # Statically linking raylib works, but is slower by 0.3 seconds.
   # LIBS="\
   # dependencies/raylib/src/libraylib.a \
   # -framework Cocoa -framework OpenGL -framework IOKit \
   # "
fi

build_raylib()
{
   cd dependencies/raylib/src \
   && make clean \
   && make PLATFORM=PLATFORM_DESKTOP CUSTOM_CFLAGS="-D SUPPORT_CUSTOM_FRAME_CONTROL"
}

build_tracyserver()
{
   cd dependencies/tracy/profiler/build/unix \
   && CPATH=/opt/local/include/capstone LIBRARY_PATH=/opt/local/lib make -j
}

build_tracyclient()
{
   c++ -std=c++11 -g3 -O3 -D TRACY_ENABLE -march=native -w -c dependencies/tracy/public/TracyClient.cpp
   mv TracyClient.o dependencies
}

if [ $1 = "raylib" ]; then
   build_raylib
   exit
fi
if [ $1 = "tracyserver" ]; then
   build_tracyserver
   exit
fi
if [ $1 = "tracyclient" ]; then
   build_tracyclient
   exit
fi

if [ $1 = "debug" ]; then
   CFLAGS="$CFLAGS -O0"
elif [ $1 = "sanitize" ]; then
   CC="/opt/local/bin/clang++-mp-16"
   CFLAGS="$CFLAGS -fsanitize=address -fno-omit-frame-pointer -O3"
elif [ $1 = "profile" ]; then
   CFLAGS="$CFLAGS -O3 -D TRACY_ENABLE -march=native"
   LIBS="$LIBS dependencies/TracyClient.o"
elif [ $1 = "tests" ]; then
   $CC $CFLAGS $WARNINGS $INCLUDES -o tests source_code/tests.cpp $LIBS
   exit
else
   echo "unrecognized build config: '$1'"
   exit 1
fi

set -xe

$CC $CFLAGS $WARNINGS $INCLUDES -o main source_code/main.cpp $LIBS
