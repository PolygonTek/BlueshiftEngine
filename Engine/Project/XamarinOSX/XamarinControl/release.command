#! /bin/bash
cd `dirname $0`

make release
mkdir libs

ln -f ../../../Library/OSX/Release/libControl.dylib libs/libControl.dylib

echo done
