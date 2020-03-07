@echo off
pushd %~dp0
ruby genggl.rb compatibility 1.2 Compatibility
popd
pause