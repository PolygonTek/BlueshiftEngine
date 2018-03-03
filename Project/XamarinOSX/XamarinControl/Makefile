XBUILD=/Applications/Xcode.app/Contents/Developer/usr/bin/xcodebuild
PROJECT_DIR=../../../Build/xcode-OSX
PROJECT=$(PROJECT_DIR)/BlueshiftEngine.xcodeproj
LIBRARY_DIR_d=../../../Library/OSX/Debug
LIBRARY_DIR=../../../Library/OSX/Release
BTOUCH=/Developer/MonoTouch/usr/bin/btouch-native

all: debug


$(LIBRARY_DIR_d)/libControl.dylib:
	$(XBUILD) -project $(PROJECT) -target libControl -arch x86_64 -configuration Debug build

$(LIBRARY_DIR)/libControl.dylib:
	$(XBUILD) -project $(PROJECT) -target libControl -arch x86_64 -configuration Release build


debug: $(LIBRARY_DIR_d)/libControl.dylib

release: $(LIBRARY_DIR)/libControl.dylib

clean:
	-rm -rf $(LIBRARY_DIR_d)/
	-rm -rf $(LIBRARY_DIR)/
	$(XBUILD) -project $(PROJECT) -target libControl -arch x86_64 -configuration Debug clean
	$(XBUILD) -project $(PROJECT) -target libControl -arch x86_64 -configuration Release clean
