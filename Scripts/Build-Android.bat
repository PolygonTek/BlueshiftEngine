pushd %~dp0

call "%programfiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
cd ..\Engine\Project\JavaAndroid

rd /S /Q AndroidPlayer\build
rd /S /Q AndroidPlayer\app\build

rd /S /Q AndroidPlayer\ARM
rd /S /Q AndroidPlayerLibrary\ARM
rd /S /Q BlueshiftEngine\ARM
rd /S /Q Bullet\ARM
rd /S /Q Etc2CompLib\ARM
rd /S /Q etcpack_lib\ARM
rd /S /Q freetype\ARM
rd /S /Q HACD\ARM
rd /S /Q jsoncpp\ARM
rd /S /Q libjpeg\ARM
rd /S /Q libogg\ARM
rd /S /Q libpng\ARM
rd /S /Q libpvrt\ARM
rd /S /Q libvorbis\ARM
rd /S /Q lua\ARM
rd /S /Q LuaCpp\ARM
rd /S /Q luasocket\ARM
rd /S /Q minizip\ARM
rd /S /Q nvidia\ARM
rd /S /Q NvTriStrip\ARM
rd /S /Q zlib\ARM

REM msbuild /v:m /m BlueshiftEngine.sln /p:Configuration=Release /p:Platform=Tegra-Android
devenv JavaAndroid.sln /build "Debug|ARM" /project "AndroidPlayer\AndroidPlayer.androidproj"
devenv JavaAndroid.sln /build "Release|ARM" /project "AndroidPlayer\AndroidPlayer.androidproj"

popd
pause