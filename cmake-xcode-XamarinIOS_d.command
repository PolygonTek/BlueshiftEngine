#! /bin/bash
cd `dirname $0`

if [ ! -d "Build" ]; then
	mkdir Build
fi
cd Build
if [ -d "XamarinIOS_d" ]; then
	rm -Rf XamarinIOS_d
fi
mkdir XamarinIOS_d
cd XamarinIOS_d
cmake -DCMAKE_TOOLCHAIN_FILE="CMake/iOS.toolchain.cmake" -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -DXAMARIN=1 -DWITHDEBUG=1 -G "Xcode" ../..
