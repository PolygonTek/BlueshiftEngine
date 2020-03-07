@echo off
pushd %~dp0
ruby genggl.rb core 3.2 Core32
popd
pause