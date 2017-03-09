#! /bin/bash
cd `dirname $0`
$QT_DIR/Bin/macdeployqt ../Engine/Bin/macOS/Release/BlueshiftEditor.app -no-plugins
