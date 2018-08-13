@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2017" rd "msvc2017" /s /q
md msvc2017
cd msvc2017
cmake -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -DBUILD_EDITOR=1 -G "Visual Studio 15 2017 Win64" ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2017
@echo:
popd
if "%1" == "nopause" goto end
pause
:end