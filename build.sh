#!/bin/sh

CC=cc
CFLAGS="-g3"
WARNINGS="\
-Wall \
-Wextra \
-Wstrict-prototypes \
-Wdouble-promotion \
-Wno-unused-parameter \
-Wno-unused-function \
-Wconversion \
-Wno-sign-conversion \
"
INCLUDES="\
-I/opt/local/include \
-I/opt/local/include/julia \
"
LIBS="\
-L/opt/local/lib \
-lraylib \
-ljulia \
-Wl,-rpath,/opt/local/lib \
dependencies/raygui.dylib \
"

build_raygui() {
   cp dependencies/raygui-3.6/src/raygui.h dependencies/raygui.c
   cc -o dependencies/raygui.dylib dependencies/raygui.c -shared -fpic -DRAYGUI_IMPLEMENTATION -framework OpenGL -lm -lpthread -ldl $(pkg-config --libs --cflags raylib)
}

build_tracyserver()
{
   pushd dependencies/tracy/profiler/build/unix
   CPATH=/opt/local/include/capstone LIBRARY_PATH=/opt/local/lib make -j
   popd
}

build_tracyclient()
{
   c++ -std=c++11 -g3 -O3 -D TRACY_ENABLE -march=native -w -c dependencies/tracy/public/TracyClient.cpp
   mv TracyClient.o dependencies
}

if [ $1 = "raygui" ]; then
   build_raygui
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
   CC="/opt/local/bin/clang-mp-16"
   CFLAGS="$CFLAGS -fsanitize=address -fno-omit-frame-pointer -O3"
elif [ $1 = "profile" ]; then
   CFLAGS="$CFLAGS -O3 -D TRACY_ENABLE -march=native"
   LIBS="$LIBS dependencies/TracyClient.o"
   CC="c++ -std=c++11"
else
   echo "unrecognized build config: '$1'"
   exit 1
fi

set -xe

$CC $CFLAGS $WARNINGS $INCLUDES -o main source_code/main.c $LIBS
