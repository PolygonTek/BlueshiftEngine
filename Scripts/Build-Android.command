#!/usr/bin/env bash

cd `dirname $0`
cd ../Engine/Build/Android/BlueshiftEngine
./gradlew BlueshiftPlayer:assembleRelease
