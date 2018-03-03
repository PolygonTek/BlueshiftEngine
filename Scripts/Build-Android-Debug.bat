@pushd %~dp0
@cd ..\Engine\Build\Android\BlueshiftEngine
gradlew.bat BlueshiftPlayer:assembleDebug
@popd
@pause