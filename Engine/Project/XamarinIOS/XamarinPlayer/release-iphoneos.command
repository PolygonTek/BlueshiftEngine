#! /bin/bash
cd `dirname $0`

make release-iphoneos
mkdir -p libs

ln -f ../../../Library/XamarinIOS-iphoneos/Release/BlueshiftPlayer.a libs/libBlueshiftPlayer.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/BlueshiftEngine.a libs/libBlueshiftEngine.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/minizip.a libs/libminizip.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/libjpeg.a libs/liblibjpeg.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/libpng.a libs/liblibpng.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/zlib.a libs/libzlib.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/libpvrt.a libs/liblibpvrt.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/libvorbis.a libs/liblibvorbis.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/libogg.a libs/liblibogg.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/jsoncpp.a libs/libjsoncpp.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/freetype.a libs/libfreetype.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/Bullet.a libs/libBullet.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/HACD.a libs/libHACD.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/LuaCpp.a libs/libLuaCpp.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/lua.a libs/liblua.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/luasocket.a libs/libluasocket.a
ln -f ../../../Library/XamarinIOS-iphoneos/Release/NvTriStrip.a libs/libNvTriStrip.a

make btouch
