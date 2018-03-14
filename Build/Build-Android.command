#!/usr/bin/env bash

cd `dirname $0`
cd Android/Blueshift
./gradlew app:assembleRelease
