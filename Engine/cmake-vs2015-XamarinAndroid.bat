@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2015-XamarinAndroid" rd "msvc2015-XamarinAndroid" /s /q
md msvc2015-XamarinAndroid
cd msvc2015-XamarinAndroid
cmake -DCMAKE_TOOLCHAIN_FILE="CMake/Android.toolchain.cmake" -DBUILD_ENGINE=1 -DBUILD_PLAYER=1 -DANDROID=1 -DXAMARIN=1 ../..
cd ..
ren msvc2015-XamarinAndroid msvc2015-XamarinAndroid-temp
@echo:
@echo Solution created in %~dp0Build\msvc2015-XamarinAndroid
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
