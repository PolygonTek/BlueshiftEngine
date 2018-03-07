#!/usr/bin/env bash

cd `dirname $0`
cd ..
./cmake-xcode-macOS-Editor.command
cd Build/xcode-macOS
xcodebuild -project BlueshiftEngine.xcodeproj -destination 'platform=OS X,arch=x86_64' -configuration Release