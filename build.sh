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
-Wl,-rpath,/opt/local/lib
"

if [ $1 = "debug" ]; then
   CFLAGS="$CFLAGS -O0"
elif [ $1 = "sanitize" ]; then
   CC="/opt/local/bin/clang-mp-16"
   CFLAGS="$CFLAGS -fsanitize=address -fno-omit-frame-pointer -O3"
elif [ $1 = "release" ]; then
   CFLAGS="$CFLAGS -O3"
else
   echo "unrecognized build config: '$1'"
   exit 1
fi

set -xe

$CC $CFLAGS $WARNINGS $INCLUDES -o main source_code/main.c $LIBS
