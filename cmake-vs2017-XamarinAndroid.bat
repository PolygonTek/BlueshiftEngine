@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2017-XamarinAndroid" rd "msvc2017-XamarinAndroid" /s /q
md msvc2017-XamarinAndroid
cd msvc2017-XamarinAndroid
cmake -DCMAKE_TOOLCHAIN_FILE="CMake/Android.toolchain.cmake" -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -DANDROID=1 -DXAMARIN=1 ../..
@echo:
@echo Solution created in %~dp0Build\msvc2017-XamarinAndroid
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
