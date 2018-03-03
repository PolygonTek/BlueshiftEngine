#! /bin/bash
cd `dirname $0`

make debug-iphoneos
mkdir -p libs

ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/BlueshiftPlayer.a libs/libBlueshiftPlayer.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/BlueshiftEngine.a libs/libBlueshiftEngine.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/minizip.a libs/libminizip.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/libjpeg.a libs/liblibjpeg.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/libpng.a libs/liblibpng.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/zlib.a libs/libzlib.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/libpvrt.a libs/liblibpvrt.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/libvorbis.a libs/liblibvorbis.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/libogg.a libs/liblibogg.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/jsoncpp.a libs/libjsoncpp.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/freetype.a libs/libfreetype.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/Bullet.a libs/libBullet.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/HACD.a libs/libHACD.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/LuaCpp.a libs/libLuaCpp.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/lua.a libs/liblua.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/luasocket.a libs/libluasocket.a
ln -f ../../../Library/XamarinIOS-iphoneos_d/Release/NvTriStrip.a libs/libNvTriStrip.a

make btouch

