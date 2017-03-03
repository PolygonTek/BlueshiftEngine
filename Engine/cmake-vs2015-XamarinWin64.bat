@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "XamarinWin64" rd "XamarinWin64" /s /q
md XamarinWin64
cd XamarinWin64
cmake -DUSE_LUAJIT=0 -DBUILD_ENGINE=1 -DBUILD_EDITOR=1 -DXAMARIN=1 -G "Visual Studio 14 2015 Win64" ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2015
@echo:
popd
if "%1" == "nopause" goto end
pause
:end