#!/usr/bin/env bash

cd `dirname $0`
cd ..
./cmake-xcode-iOS.command
cd Build/xcode-iOS
xcodebuild -project Blueshift.xcodeproj ONLY_ACTIVE_ARCH=NO -destination 'generic/platform=iOS' -configuration Debug CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO

