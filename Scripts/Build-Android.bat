pushd %~dp0
cd ..\Engine
REM call cmake-vs2015-Android.bat nopause
call "%VS140COMNTOOLS%..\..\VC\bin\amd64\vcvars64.bat"
REM cd Build\msvc2015-android
REM msbuild /v:m /m BlueshiftEngine.sln /p:Configuration=Release /p:Platform=Tegra-Android
cd Project\Android
devenv AndroidPlayer.sln /build "Release|Tegra-Android" /project "AndroidPlayer\AndroidPlayer.vcxproj"
popd
pause