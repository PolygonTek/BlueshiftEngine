Build Sample Program
-------------------

## Prerequisite

  * Build-Android.md 
  * README.md 의 iOS 버젼

## Game 빌드 테스트

	EngineData/BasicGame 파일을 받아 도큐먼트 폴더(~/Documents) 에 복사한다.

	에디터(BlueshiftEngine/Engine/Bin/Win64\/Release/BlueshiftEditor.exe)에서 File/Open Project 메뉴에서 ~/Document/BasicGame 폴더를 선택하고,
	File/Open Scene 메뉴에서 ~/Documents/BasicGame/Contents/Maps/stage1.map 을 선택한다.

	Project/Build Settings... 메뉴에서 플랫폼을 선택하고 빌드를 하면 빌드 폴더를
	~/Document/BasicGameAnd (BasicGameIOS, BasicGameWin, ...) 으로 지정한다.

	프로젝트 빌드가 끝나면 지정한 폴더의 BasicGameAnd 프로젝트 폴더가 자동으로 열린다.


##Debugging Tips for mobile platform
-----------------------------

### Android

%USERPROFILE%\Documents\BasicGameAnd\Project\Android\AndroidPlayer\assets\(Data|Config)를
안드로이드 장비의 /sdcard/blueshift 밑으로 복사
BlueshiftEngine/Engine/Project/Android/AndroidPlayer.sln을 열고, AndroidPlayer를 실행한다.

### iOS

~/Documents/BasicGameIOS/(Data|Config)를 BlueshiftEngine/Engine/Source/Player밑으로 복사
BlueshiftEngine/Engine/cmake-xcode-iOS.command를 실행한다.
BlueshiftEngine/Engine/Build/xcode-iOS/BlueshiftEngine.xcodeproj를 열고, Player를 실행한다.

