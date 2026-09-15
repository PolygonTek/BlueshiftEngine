@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2026" rd "msvc2026" /s /q
md msvc2026
cd msvc2026
cmake -DBUILD_RUNTIME=1 -DBUILD_TEST=1 -DBUILD_TESTD3D12=1 -DBUILD_TESTTRACER=1 -DBUILD_PLAYER=1 -DBUILD_EDITOR=1 -G "Visual Studio 18 2026" -A x64 ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2026
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
