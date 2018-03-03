copy C:\ProgramData\Microsoft\AndroidNDK\android-ndk-r11c\prebuilt\android-arm\gdbserver\gdbserver  %USERPROFILE%\Documents\blueshift\Engine\Project\XamarinAndroid\XamarinPlayer/libs/armeabi-v7a/gdbserver
XamarinPlayer/libs/armeabi-v7a/libsangeles.so/Build Action:AndroidNativeLibrary
XamarinPlayer/libs/armeabi-v7a/gdbserver/Build Action:AndroidNativeLibrary
XamarinPlayerLibrary/PostBuildEvent:
#	copy $(IntermediateOutputPath)libXamarinPlayer.so %USERPROFILE%\Documents\android
	echo copy $(IntermediateOutputPath)libXamarinPlayer.so ..\XamarinPlayer\bin\$(Configuration)
	copy $(IntermediateOutputPath)libXamarinPlayer.so ..\XamarinPlayer\bin\$(Configuration)
	echo copy $(OutputPath)libXamarinPlayer.so ..\XamarinPlayer\libs\armeabi-v7a
	copy $(OutputPath)libXamarinPlayer.so ..\XamarinPlayer\libs\armeabi-v7a


LD_LIBRARY_PATH: %USERPROFILE%\Documents\android;


Android ndk debug
=========================

mkdir %USERPROFILE%\Documents\android
cd %USERPROFILE%\Documents\android
adb shell ls -l /system/bin/app_process
adb pull /system/bin/app_process  
adb pull /system/bin/linker  


SanAngele Project > Properties > Android Options > Debugging optoins > Xamarin
adb shell "ps | grep SanAngeles"
adb> su
adb> cat /proc/21223/cmdline
adb forward tcp:5039 localfilesystem:/data/data/SanAngeles.SanAngeles/debug-pipe
adb shell run-as SanAngeles.SanAngeles /data/data/SanAngeles.SanAngeles/lib/gdbserver +debug-pipe --attach 23467 
cd C:\NVPACK\android-ndk-r10e\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin
arm-linux-androideabi-gdb %USERPROFILE%\Documents\android\app_process 
gdb> target remote :5039
# gdb> set solib-search-path ~/Documents/android;~/Documents/android/system_lib:~/Documents/android/vendor_lib
gdb> info sharedlibrary 



Mono San Angeles sample port
============================

This is a port of Android NDK sample (sanangeles).
http://developer.android.com/tools/sdk/ndk/index.html#Samples


This library sample is easily ported because there is no dependency on
any jobject instances or JNIEnv instance in C code.

There are **TWO** versions available:


1) C++ for Visual Studio 2015 (SanAngeles_NativeDebug)
---

Complete C++ source is built as part of the solution, which means you can debug the C++ code while running the Xamarin.Android project using **Visual Studio 2015**.



2) Pre-compiled (SanAngeles_NDK)
-----

The project contains pre-compiled libsanangeles.so under libs directory
(armeabi, armeabi-v7a and x86 are supported). You'll only debug the Xamarin.Android C# code with this example.




S
