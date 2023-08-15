#!/bin/sh

set -xe

/usr/bin/time -o buildtime.txt ./build.sh debug
