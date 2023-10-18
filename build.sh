#!/bin/sh

# use_julia_backend=true
use_julia_backend=false

CC=c++
CFLAGS="-o main -std=c++11 -g3"

WARNINGS="\
-Wall \
-Wextra \
-Wdouble-promotion \
-Wno-unused-parameter \
-Wno-unused-function \
-Wconversion \
-Wno-sign-conversion \
"

if [ "$use_julia_backend" = true ]; then
   CFLAGS="$CFLAGS -D JULIA_BACKEND"
fi

OS=`uname`
echo "Operating system: $OS"
PROJECTDIR="$PWD"

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
   -Idependencies/raylib/src \
   "
   LIBS="\
   -L/opt/local/lib \
   -lraylib \
   dependencies/rlImGui/rlImGui.o \
   dependencies/imgui/imgui.o \
   dependencies/imgui/imgui_draw.o \
   dependencies/imgui/imgui_tables.o \
   dependencies/imgui/imgui_widgets.o \
   "
   # dependencies/raylib/src/libraylib.a -framework Cocoa -framework OpenGL -framework IOKit \

   if [ "$use_julia_backend" = true ]; then
      INCLUDES="$INCLUDES -I/opt/local/include/julia"
      LIBS="$LIBS -ljulia -Wl,-rpath,/opt/local/lib"
   fi
fi

build_raylib()
{
   set -xe
   cd dependencies/raylib/src
   # make clean
   # make PLATFORM=PLATFORM_DESKTOP
   # && make PLATFORM=PLATFORM_DESKTOP CUSTOM_CFLAGS="-D SUPPORT_CUSTOM_FRAME_CONTROL"

   # https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5)#21-command-line-compilation
   emcc -c rcore.c -o rcore.o.wasm -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
   emcc -c rshapes.c -o rshapes.o.wasm -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
   emcc -c rtextures.c -o rtextures.o.wasm -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
   emcc -c rtext.c -o rtext.o.wasm -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
   emcc -c rmodels.c -o rmodels.o.wasm -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
   emcc -c utils.c -o utils.o.wasm -Os -DPLATFORM_WEB
   emcc -c raudio.c -o raudio.o.wasm -Os -DPLATFORM_WEB
}

build_imgui()
{
   set -xe
   cd dependencies/imgui
   c++ -std=c++11 -g3 -O3 -c imgui.cpp -o imgui.o
   c++ -std=c++11 -g3 -O3 -c imgui_draw.cpp -o imgui_draw.o
   c++ -std=c++11 -g3 -O3 -c imgui_widgets.cpp -o imgui_widgets.o
   c++ -std=c++11 -g3 -O3 -c imgui_tables.cpp -o imgui_tables.o
   c++ -std=c++11 -g3 -O3 -c imgui_demo.cpp -o imgui_demo.o
   em++ -std=c++11 -g3 -O3 -c imgui.cpp -o imgui.o.wasm
   em++ -std=c++11 -g3 -O3 -c imgui_draw.cpp -o imgui_draw.o.wasm
   em++ -std=c++11 -g3 -O3 -c imgui_widgets.cpp -o imgui_widgets.o.wasm
   em++ -std=c++11 -g3 -O3 -c imgui_tables.cpp -o imgui_tables.o.wasm
   em++ -std=c++11 -g3 -O3 -c imgui_demo.cpp -o imgui_demo.o.wasm
}

build_rlimgui()
{
   set -xe
   cd dependencies/rlImGui
   c++ -std=c++11 -g3 -O3 -iquote ../raylib/src -iquote ../imgui -c rlImGui.cpp -o rlImGui.o
   em++ -std=c++11 -g3 -O3 -iquote ../raylib/src -iquote ../imgui -c rlImGui.cpp -o rlImGui.o.wasm
}

build_tracyserver()
{
   set -xe
   cd dependencies/tracy/profiler/build/unix \
   && CPATH=/opt/local/include/capstone LIBRARY_PATH=/opt/local/lib make -j
}

build_tracyclient()
{
   set -xe
   c++ -std=c++11 -g3 -O3 -D TRACY_ENABLE -march=native -w -c dependencies/tracy/public/TracyClient.cpp
   mv TracyClient.o dependencies
}

if [ $1 = "raylib" ]; then
   build_raylib
   exit
fi
if [ $1 = "imgui" ]; then
   build_imgui
   exit
fi
if [ $1 = "rlimgui" ]; then
   build_rlimgui
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

elif [ $1 = "release" ]; then
   CFLAGS="$CFLAGS -O3"

elif [ $1 = "sanitize" ]; then
   CC="/opt/local/bin/clang++-mp-16"
   CFLAGS="$CFLAGS -fsanitize=address -fno-omit-frame-pointer -O3"

elif [ $1 = "profile" ]; then
   CFLAGS="$CFLAGS -O3 -D TRACY_ENABLE -march=native"
   LIBS="$LIBS dependencies/TracyClient.o"

elif [ $1 = "tests" ]; then
   set -xe
   LIBS="$LIBS dependencies/imgui/imgui_demo.o"
   $CC $CFLAGS $WARNINGS $INCLUDES -o tests source_code/tests.cpp $LIBS
   exit

elif [ $1 = "web" ]; then
   CC=em++
   CFLAGS="-D WEB -o index.html -s USE_GLFW=3 --shell-file shell-minimal.html"
   INCLUDES="\
   -I dependencies/raylib/src \
   "
   LIBS="
   dependencies/raylib/src/rcore.o.wasm \
   dependencies/raylib/src/rshapes.o.wasm \
   dependencies/raylib/src/rtextures.o.wasm \
   dependencies/raylib/src/rtext.o.wasm \
   dependencies/raylib/src/rmodels.o.wasm \
   dependencies/raylib/src/utils.o.wasm \
   dependencies/raylib/src/raudio.o.wasm \
   dependencies/rlImGui/rlImGui.o.wasm \
   dependencies/imgui/imgui.o.wasm \
   dependencies/imgui/imgui_draw.o.wasm \
   dependencies/imgui/imgui_tables.o.wasm \
   dependencies/imgui/imgui_widgets.o.wasm \
   "

else
   echo "unrecognized build config: '$1'"
   exit 1
fi

set -xe

# $CC $CFLAGS $WARNINGS $INCLUDES source_code/main.cpp $LIBS
$CC $CFLAGS $WARNINGS $INCLUDES source_code/main2.cpp $LIBS

# for itch.io
if [ $1 = "web" ]; then
   rm -rf gamehtml.zip
   zip -r gamehtml.zip index.html index.js index.wasm assets/
fi
