Build Sample Program
--------------------

## Prerequisite

  * [Build-Android.md](Build-Android.md)
  * [README.md](README.md) 중에서 iOS 부분

## Game 빌드 테스트
-------------------
[BasicGame.zip](https://github.com/PolygonTek/BlueshiftDocument/raw/master/BasicGame.zip) 파일을 받아 
도큐먼트 폴더(~/Documents) 에 압축을 푼다.

에디터(BlueshiftEngine/Engine/Bin/Win64/Release/BlueshiftEditor.exe)를 실행하고,
File/Open Project 메뉴에서 ~/Document/BasicGame 폴더를 선택하고,
File/Open Scene 메뉴에서 ~/Documents/BasicGame/Contents/Maps/title.map 을 선택한다.

Project/Build Settings... 메뉴에서 플랫폼을 선택하고 빌드를 하면 빌드 폴더를
~/Document/BasicGameAnd (BasicGameIOS, BasicGameWin, ...) 으로 지정한다.

프로젝트 빌드가 끝나면 지정한 폴더의 BasicGameAnd 프로젝트 폴더가 자동으로 열린다.


## 참고사항
___________

[Build iOS](https://github.com/PolygonTek/BlueshiftDocument/blob/master/Build%20iOS.pdf)


##Debugging Tips for mobile platform
------------------------------------

### Android

%USERPROFILE%\Documents\BasicGameAnd\Project\Android\AndroidPlayer\assets 밑의 Data와 Config를
안드로이드 장비의 /sdcard/blueshift 밑으로 복사
BlueshiftEngine/Engine/Project/Android/AndroidPlayer.sln을 열고, AndroidPlayer를 실행한다.

### iOS

~/Documents/BasicGameIOS 밑의 Data와 Config를 BlueshiftEngine/Engine/Source/Player 밑으로 복사
BlueshiftEngine/Engine/Build/xcode-iOS/BlueshiftEngine.xcodeproj를 열고, BlueshiftPlayer를 실행한다.

