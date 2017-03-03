pushd %~dp0
cd Engine
call cmake-vs2015-Win64.bat nopause
call "%VS140COMNTOOLS%..\..\VC\bin\amd64\vcvars64.bat"
cd Build\msvc2015
msbuild /v:m /m BlueshiftEngine.sln /p:Configuration=Release /p:Platform=x64
popd
pause