#! /bin/bash
cd `dirname $0`

make release-iphonesimulator
mkdir -p libs


ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/BlueshiftPlayer.a libs/libBlueshiftPlayer.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/BlueshiftEngine.a libs/libBlueshiftEngine.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/minizip.a libs/libminizip.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/libjpeg.a libs/liblibjpeg.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/libpng.a libs/liblibpng.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/zlib.a libs/libzlib.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/libpvrt.a libs/liblibpvrt.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/libvorbis.a libs/liblibvorbis.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/libogg.a libs/liblibogg.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/jsoncpp.a libs/libjsoncpp.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/freetype.a libs/libfreetype.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/Bullet.a libs/libBullet.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/HACD.a libs/libHACD.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/LuaCpp.a libs/libLuaCpp.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/lua.a libs/liblua.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/luasocket.a libs/libluasocket.a
ln -f ../../../Library/XamarinIOS-iphonesimulator/Release/NvTriStrip.a libs/libNvTriStrip.a

make btouch
