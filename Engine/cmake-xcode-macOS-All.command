#! /bin/bash
cd `dirname $0`

if [ ! -d "Build" ]; then
	mkdir Build
fi
cd Build
if [ -d "xcode-macOS" ]; then
	rm -Rf xcode-macOS
fi
mkdir xcode-macOS
cd xcode-macOS
cmake -DBUILD_ENGINE=1 -DBUILD_TEST=1 -DBUILD_PLAYER=1 -DBUILD_EDITOR=1 -G "Xcode" ../..
