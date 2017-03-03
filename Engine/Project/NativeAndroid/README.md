
LD_LIBRARY_PATH: %USERPROFILE%\Documents\android;

AndroidPlayerLibrary/PostBuildEvent:
	copy $(IntermediateOutputPath)libAndroidPlayer.so %USERPROFILE%\Documents\android