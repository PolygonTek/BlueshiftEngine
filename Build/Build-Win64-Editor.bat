@echo off
setlocal EnableExtensions EnableDelayedExpansion

pushd "%~dp0.."

if "%VSWHERE%"=="" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
	echo ERROR: vswhere.exe not found: "%VSWHERE%"
	set "BuildResult=1"
	goto finish
)

set "BuildResult=0"
set "SelectedVSPath="

set "VSVersionRange=[18.0,19.0)"
for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -version "!VSVersionRange!" -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SelectedVSPath=%%i"
if defined SelectedVSPath (
	set "CMakeBatch=cmake-vs2026-Win64-Editor.bat"
	set "BuildDir=Build\msvc2026"
	set "SolutionFile=Blueshift.slnx"
	goto generate
)

set "VSVersionRange=[17.0,18.0)"
for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -version "!VSVersionRange!" -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SelectedVSPath=%%i"
if defined SelectedVSPath (
	set "CMakeBatch=cmake-vs2022-Win64-Editor.bat"
	set "BuildDir=Build\msvc2022"
	set "SolutionFile=Blueshift.sln"
	goto generate
)

echo ERROR: Visual Studio 2026 or Visual Studio 2022 with MSBuild and C++ tools is required.
set "BuildResult=1"
goto finish

:generate
call "%CMakeBatch%" nopause
if not exist "%BuildDir%\%SolutionFile%" (
	echo ERROR: CMake failed to generate "%BuildDir%\%SolutionFile%".
	set "BuildResult=1"
	goto finish
)

call "%SelectedVSPath%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
	echo ERROR: Failed to initialize the Visual Studio build environment.
	set "BuildResult=1"
	goto finish
)

pushd "%BuildDir%"
msbuild /v:m /m "%SolutionFile%" /p:Configuration=Release /p:Platform=x64
set "BuildResult=%ERRORLEVEL%"
popd

if not "%BuildResult%"=="0" echo ERROR: Build failed with exit code %BuildResult%.

:finish
popd
if not "%~1"=="nopause" pause
endlocal & exit /b %BuildResult%
