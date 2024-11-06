@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2022clang" rd "msvc2022clang" /s /q
md msvc2022clang
cd msvc2022clang
cmake -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -G "Visual Studio 17 2022" -A "x64" -T ClangCL ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2022clang
@echo:
popd
if "%1" == "nopause" goto end
pause
:end
