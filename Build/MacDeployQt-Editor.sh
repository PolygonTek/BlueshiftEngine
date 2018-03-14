#! /bin/bash
cd `dirname $0`
cd ../Bin/macOS/Release
$QT_DIR/Bin/macdeployqt BlueshiftEditor.app -executable="BlueshiftEditor.app/Contents/MacOS/BlueshiftEditor" -always-overwrite -verbose=2 -dmg
