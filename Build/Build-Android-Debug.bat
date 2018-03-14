@pushd %~dp0
@cd Android\Blueshift
cmd /k gradlew.bat app:assembleDebug
@popd
