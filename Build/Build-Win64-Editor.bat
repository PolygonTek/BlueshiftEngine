@pushd %~dp0
@cd ..
@if "%VSWHERE%"=="" set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
@if exist "%VSWHERE%" (
	@for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  		@set VSInstallDir=%%i
	)
)
@if defined VSInstallDir (
	@call cmake-vs2017-Win64-Editor.bat nopause
	@pushd "%~dp0" 
	@call "%VSInstallDir%\VC\Auxiliary\Build\vcvars64.bat"
	@popd
	@cd Build\msvc2017
) else ( 
	@if defined VS140COMNTOOLS (
		@call cmake-vs2015-Win64-Editor.bat nopause
		@call "%VS140COMNTOOLS%..\..\VC\bin\amd64\vcvars64.bat"
		@cd Build\msvc2015
	)
)
msbuild /v:m /m Blueshift.sln /p:Configuration=Release /p:Platform=x64
@popd
@pause