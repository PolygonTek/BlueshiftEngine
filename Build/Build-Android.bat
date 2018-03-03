@pushd %~dp0
@cd Android\Blueshift
gradlew.bat BlueshiftPlayer:assembleRelease
@popd
@pause