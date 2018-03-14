
Build > custom command > prebuild : debug-iphoneos.command
Build > iOS Build > mtouch : 
-cxx -gcc_flags "-L${ProjectDir}/libs -lluasocket -lBlueshiftPlayer -lBlueshiftEngine -lminizip -llibjpeg -llibpng -lzlib -llibpvrt -llibvorbis -llibogg -ljsoncpp -lfreetype -lBullet -lHACD -lLuaCpp -llua -lluasocket -lNvTriStrip -framework OpenAL -framework AudioToolbox -framework AVFoundation"
Build > iOS Build > optimize PNG image : off
Build > iOS Build > Strip Native Debugging Symbols : off
===========================
release compile option : 
Deployment/Strip Debug Symbols During Copy
Apple LLVM 7.1 Code Generation/Optimization Level : -Os - > -O0

Architexures/Architextures : armv7;arm64
Architextres/Build Active Architecture Only : No

File/Project Setting.../Project-relative
===============================


http://developer.xamarin.com/guides/ios/advanced_topics/native_interop/
xamarin Project/Option/Build/iOS Build/Additional Options : 
    -cxx -gcc_flags "-L${ProjectDir} -lXMBindingLibrarySampleUniversal  -force_load ${ProjectDir}/libXMBindingLibrarySampleUniversal.a"


rm XMBindingLibrarySample/build
nm libXMBindingLibrarySample-i386.a 

Xcode/Product/Scheme/Run/Executable:
/Users/sjpark/Library/Developer/CoreSimulator/Devices/25868BB6-DDC4-4002-AF3C-6086649F2A12/data/Containers/Bundle/Application/BD0E31C4-861E-40F8-BE17-C0C464E4927E/XamarinXMBindingLibrarySample.app/XamarinXMBindingLibrarySample
iphone-Simulator  iphone 4s


ps -ef | grep XamarinPlayer
kill pid

lldb 
(lldb) process attach -n XamarinPlayer --waitfor
(lldb) b mono_assembly_init_with_opt    
(lldb) c
# when the thread breaks
(lldb) thread return 0
(lldb) c
(lldb) detach







