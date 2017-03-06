#!/usr/bin/env bash

cd `dirname $0`
cd ../Engine
./cmake-xcode-iOS.command
cd Build/xcode-iOS
xcodebuild -project BlueshiftEngine.xcodeproj ONLY_ACTIVE_ARCH=NO -destination 'generic/platform=iOS' -configuration Debug CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
xcodebuild -project BlueshiftEngine.xcodeproj ONLY_ACTIVE_ARCH=NO -destination 'generic/platform=iOS' -configuration Release CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO