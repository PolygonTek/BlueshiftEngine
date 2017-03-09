Build Sample Program
--------------------

## Prerequisite

  * [Build-Android.md](Build-Android.md)
  * iOS part of [README.md](README.md)

## Game Build Test

Download the file [BasicGame.zip](https://github.com/PolygonTek/BlueshiftDocument/raw/master/BasicGame/BasicGame.zip),
extract to the `~/Documents` folder

Run `BlueshiftEditor` (`BlueshiftEngine/Engine/Bin/Win64/Release/BlueshiftEditor.exe`),
select the `~/Document/BasicGame` folder from the `File/Open Project` menu,
and select the `~/Documents/BasicGame/Contents/Maps/title.map` from the `File/Open Scene` menu.


Select the platform from the `Project/Build Settings...` menu,
set build folder  to `~/Document/BasicGameAnd (BasicGameIOS, BasicGameWin, ...)`.

When the project builds, the project folder opens automatically within the build folder `BasicGameAnd`.

## See also

* [Building iOS Sample Program](https://github.com/PolygonTek/BlueshiftDocument/blob/master/Build%20iOS.pdf)


##Debugging Tips for mobile platform

### Android

Copy `data` and `config` directory under `%USERPROFILE%\Documents\BasicGameAnd\Project\Android\AndroidPlayer\assets` 
to `/sdcard/blueshift` of Android device.

Open `BlueshiftEngine/Engine/Project/Android/AndroidPlayer.sln`, then build and run `AndroidPlayer`.

### iOS

Copy `data` and `config` directory under `~/Documents/BasicGameIOS` to `BlueshiftEngine/Engine/Source/Player`.

Open `BlueshiftEngine/Engine/Build/xcode-iOS/BlueshiftEngine.xcodeproj`, then build and run `BlueshiftPlayer`.

