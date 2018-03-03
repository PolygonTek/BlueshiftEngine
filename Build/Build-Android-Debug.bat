@pushd %~dp0
@cd Android\Blueshift
gradlew.bat BlueshiftPlayer:assembleDebug
@popd
@pause