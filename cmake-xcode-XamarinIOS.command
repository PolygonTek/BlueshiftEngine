#! /bin/bash
cd `dirname $0`

if [ ! -d "Build" ]; then
	mkdir Build
fi
cd Build
if [ -d "XamarinIOS" ]; then
	rm -Rf XamarinIOS
fi
mkdir XamarinIOS
cd XamarinIOS
cmake -DCMAKE_TOOLCHAIN_FILE="CMake/iOS.toolchain.cmake" -DBUILD_ENGINE=1 -DBUILD_PLAYER=1 -DXAMARIN=1 -G "Xcode" ../..
