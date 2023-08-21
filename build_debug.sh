#!/bin/sh

set -xe

/usr/bin/time -o debugbuildtime.txt ./build.sh debug
