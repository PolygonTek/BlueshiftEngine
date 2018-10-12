pushd %~dp0
mkdir Build
cd Build
rmdir msvc2013 /s /q
mkdir msvc2013
cd msvc2013
"../../../../Bin/CMake/Windows_x86/bin/cmake.exe" -DBUILD_RUNTIME=1 -DANDROID=1 -G "Visual Studio 12 2013 Win64" ../../../..
popd
pause