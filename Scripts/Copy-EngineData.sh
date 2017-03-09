#! /bin/bash
cd `dirname $0`
cp -rpv ../EngineData/Data ../Engine
mkdir -p ../Engine/Tools/PVRTexTool
cp -rpv ../EngineData/Tools/PVRTexTool/OSX_x86 ../Engine/Tools/PVRTexTool
