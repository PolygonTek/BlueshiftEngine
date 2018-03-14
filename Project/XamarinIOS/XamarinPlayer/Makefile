XBUILD=/Applications/Xcode.app/Contents/Developer/usr/bin/xcodebuild
PROJECT_DIR=../../../Build/XamarinIOS
PROJECT_DIR_d=../../../Build/XamarinIOS_d
PROJECT=$(PROJECT_DIR)/BlueshiftEngine.xcodeproj
PROJECT_d=$(PROJECT_DIR_d)/BlueshiftEngine.xcodeproj
LIBRARY_DIR_d=../../../Library/XamarinIOS_d/Release
LIBRARY_DIR-iphoneos=../../../Library/XamarinIOS-iphoneos/Release
LIBRARY_DIR-iphoneos_d=../../../Library/XamarinIOS-iphoneos_d/Release
LIBRARY_DIR-iphonesimulator=../../../Library/XamarinIOS-iphonesimulator/Release
LIBRARY_DIR-iphonesimulator_d=../../../Library/XamarinIOS-iphonesimulator_d/Release
BTOUCH=/Developer/MonoTouch/usr/bin/btouch-native
BTOUCH_DLL=libs/BlueshiftPlayer.dll

all: debug-iphoneos


$(LIBRARY_DIR-iphoneos)/BlueshiftPlayer.a:
	$(XBUILD) -project $(PROJECT) -target BlueshiftPlayer -sdk iphoneos ARCHS="armv7 arm64" ONLY_ACTIVE_ARCH=NO -configuration Release build

$(LIBRARY_DIR-iphoneos_d)/BlueshiftPlayer.a:
	$(XBUILD) -project $(PROJECT_d) -target BlueshiftPlayer -sdk iphoneos ARCHS="armv7 arm64" ONLY_ACTIVE_ARCH=NO -configuration Release build

$(LIBRARY_DIR-iphonesimulator)/BlueshiftPlayer.a:
	$(XBUILD) -project $(PROJECT) -target BlueshiftPlayer -sdk iphonesimulator ARCHS="i386" -configuration Release build

$(LIBRARY_DIR-iphonesimulator_d)/BlueshiftPlayer.a:
	$(XBUILD) -project $(PROJECT_d) -target BlueshiftPlayer -sdk iphonesimulator ARCHS="i386" -configuration Release build

$(LIBRARY_DIR_d)/BlueshiftPlayer.a: $(LIBRARY_DIR-iphonesimulator_d)/BlueshiftPlayer.a $(LIBRARY_DIR-iphoneos_d)/BlueshiftPlayer.a
	-mkdir ../../../Library/XamarinIOS_d
	-mkdir ../../../Library/XamarinIOS_d/Release
	lipo -create -output $@ $^

debug-iphoneos: $(LIBRARY_DIR-iphoneos_d)/BlueshiftPlayer.a
	#ln -f ../../../Library/XamarinIOS_d/Release/BlueshiftPlayer.a libPlayer.a

release-iphoneos: $(LIBRARY_DIR-iphoneos)/BlueshiftPlayer.a
	#ln -f ../../../Library/XamarinIOS_d/Release/BlueshiftPlayer.a libPlayer.a

debug-iphonesimulator: $(LIBRARY_DIR-iphonesimulator_d)/BlueshiftPlayer.a
	#ln -f ../../../Library/XamarinIOS_d/Release/BlueshiftPlayer.a libPlayer.a

release-iphonesimulator: $(LIBRARY_DIR-iphonesimulator)/BlueshiftPlayer.a
	#ln -f ../../../Library/XamarinIOS_d/Release/BlueshiftPlayer.a libPlayer.a

clean:
	-rm -rf ../../../Library/XamarinIOS-iphoneos/
	-rm -rf ../../../Library/XamarinIOS-iphoneos_d/
	-rm -rf ../../../Library/XamarinIOS-iphonesimulator/
	-rm -rf ../../../Library/XamarinIOS-iphonesimulator_d/
	$(XBUILD) -project $(PROJECT) -target BlueshiftPlayer -sdk iphonesimulator -configuration Release clean
	$(XBUILD) -project $(PROJECT_d) -target BlueshiftPlayer -sdk iphonesimulator -configuration Release clean
	$(XBUILD) -project $(PROJECT) -target BlueshiftPlayer -sdk iphoneos -arch armv7 -configuration Release clean
	$(XBUILD) -project $(PROJECT_d) -target BlueshiftPlayer -sdk iphoneos -arch armv7 -configuration Release clean

btouch: $(BTOUCH_DLL)
	@echo making... $(BTOUCH_DLL)


$(BTOUCH_DLL): AssemblyInfo.cs XMBindingLibrarySample.cs extras.cs
	$(BTOUCH) -unsafe --new-style -out:$@ XMBindingLibrarySample.cs -x=AssemblyInfo.cs -x=extras.cs --link-with=libs/libBlueshiftPlayer.a,libBlueshiftPlayer.a
