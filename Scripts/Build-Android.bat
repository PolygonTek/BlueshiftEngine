pushd %~dp0
cd ..\Engine
call cmake-vs2015-Android.bat nopause
call "%VS140COMNTOOLS%..\..\VC\bin\amd64\vcvars64.bat"
cd Build\msvc2015-android
msbuild /v:m /m BlueshiftEngine.sln /p:Configuration=Release /p:Platform=Tegra-Android
popd
pause