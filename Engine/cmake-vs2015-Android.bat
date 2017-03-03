@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2015-android" rd "msvc2015-android" /s /q
md msvc2015-android
cd msvc2015-android
cmake -DCMAKE_TOOLCHAIN_FILE="CMake/Android.toolchain.cmake" -DANDROID=1 -DBUILD_ENGINE=1 -DBUILD_PLAYER=1 ../..
@echo:
@echo Solution created in %~dp0Build\msvc2015-android
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
