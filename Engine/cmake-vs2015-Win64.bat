@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2015" rd "msvc2015" /s /q
md msvc2015
cd msvc2015
cmake -DBUILD_ENGINE=1 -DBUILD_PLAYER=1 -G "Visual Studio 14 2015 Win64" ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2015
@echo:
popd
if "%1" == "nopause" goto end
pause
:end