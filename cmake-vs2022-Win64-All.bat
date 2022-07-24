@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2022" rd "msvc2022" /s /q
md msvc2022
cd msvc2022
cmake -DBUILD_RUNTIME=1 -DBUILD_TEST=1 -DBUILD_PLAYER=1 -DBUILD_EDITOR=1 -G "Visual Studio 17 2022" -A "x64" ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2022
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
