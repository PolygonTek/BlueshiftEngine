#! /bin/bash
cd `dirname $0`

make debug
mkdir libs

ln -f ../../../Library/OSX/Debug/libControl.dylib libs/libControl.dylib

echo done
