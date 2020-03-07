@echo off
pushd %~dp0
ruby genggl.rb core 4.3 Core43
popd
pause