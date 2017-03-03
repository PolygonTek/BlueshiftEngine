Xamarin based build (Experimental)
-------------------------------------

## iOS

### Prerequisite

  * Xcode
  * Xamarin for iOS 

```
`~/blueshift/Engine/cmake-xcode-XamarinIOS_d.command` 실행
`~/blueshift/Engine/Build/XamarinIOS_d/BlueshiftEngine.xcodeproj` 열기
    BlueshiftPlayer 를 Release 로 빌드
`~/blueshift/Engine/Project/XamarinIOS/XamarinPlayer.sln` 열기
    Resources 밑에 Config, Data 갱신
    빌드 후 실행

c++ debugging : menu > Debug > attach process by PID or Nmae : PID or Process Name : XamarinPlayer

```

## Android

### Prerequisite

  * [MS Visual Studio 2015](https://www.visualstudio.com/downloads/) with update 3 (select all 'Cross Platform Mobile Development' featue)

```
VS2015 > Tools > Options > Cross Platform > C++ > Android NDK > C:\ProgramData\Microsoft\AndroidNDK\android-ndk-r11c
VS2015 > Tools > Options > Cross Platform > C++ > Android SDK > C:\NVPACK\android-sdk-windows
VS2015 > Tools > Options > Xamarin > Java Development Kit Location > C:\NVPACK\jdk1.8.0_77 (%JAVA_HOME%)
VS2015 > Tools > Options > Xamarin > Android SDK Location > C:\NVPACK\android-sdk-windows
VS2015 > Tools > Options > Xamarin > Android NDK Location > C:\ProgramData\Microsoft\AndroidNDK\android-ndk-r11c

Engine\Project\XamarinAndroid\XamarinPlayer.sln VS2015로 열기
XamarinPlayerLibrary  (Debug/ARM) 빌드
XamarinPlayer (Debug/ARM) 실행

c#  debugging : XamarinPlayer > properties > Android Options > Debugging options > .Net (Xamarin)
c++ debugging : XamarinPlayer > properties > Android Options > Debugging options > C++

```
