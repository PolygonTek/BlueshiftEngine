Build Android
-------------------

### Prerequisite

  * Android part of [README.md](README.md)
  * give write permission to Android SDK Folder. "C:\Program Files (x86)\Android\adroid-sdk"/Properties/Security/Edit/Add/Everyone: Full Control
  * install Android SDK Build Tools 23.0.1. "C:\Program Files (x86)\Android\adroid-sdk\SDK Manager.exe"
  * Visual Studio 2017/Tools/Options/Cross Platform/C++/Android/Android NDK: C:\ProgramData\Microsoft\AndroidNDK64\android-ndk-r13b

### Instruction (only for C++ debugging)

  1. Open `Engine\Project\JavaAndroid\JavaAndroid.sln`

  2. Build with target `Debug ARM`

