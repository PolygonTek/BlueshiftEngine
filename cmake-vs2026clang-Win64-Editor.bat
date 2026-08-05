@echo off
@echo:
@echo Generating Visual Studio Solution (64 bit)
@echo:
pushd %~dp0
:: remove any generated cmake build files
if not exist "Build" md Build
cd "Build"
if exist "msvc2026clang" rd "msvc2026clang" /s /q
md msvc2026clang
cd msvc2026clang
cmake -DBUILD_RUNTIME=1 -DBUILD_PLAYER=1 -DBUILD_EDITOR=1 -G "Visual Studio 18 2026" -A x64 -T ClangCL ..\..
@echo:
@echo Solution created in %~dp0Build\msvc2026clang
@echo:
popd
if "%1" == "nopause" goto end
pause
:end