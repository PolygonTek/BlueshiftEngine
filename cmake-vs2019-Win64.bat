@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2019" rd "msvc2019" /s /q
md msvc2019
cd msvc2019
cmake -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -G "Visual Studio 16 2019" -A "x64" ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2019
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
