#! /bin/bash
cd `dirname $0`
$QT_DIR/Bin/macdeployqt ../Engine/Bin/macOS/Release/BlueshiftEditor.app -executable="../Engine/Bin/macOS/Release/BlueshiftEditor.app/Contents/MacOS/BlueshiftEditor" -no-plugins -always-overwrite -verbose=2
