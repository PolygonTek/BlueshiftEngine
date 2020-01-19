Version History
---------------

### 0.7.0
- Added support for 2D mode for editing GUI
- Added support for rect tool gizmo
- Added support for some GUI components (Rect, Image, Text, and WIP for others)
- Added support for drawing 9-sliced image rendering for GUI
- Added support for text rendering with shadows/outlines
- Fixed an issue where editing euler angles rotation in editor

### 0.6.0
- Refined PBR shaders
- Added support for environment probes for indirect lighting
- Added support for various HDR tonemapping operators
- Added support for integration [Google Analytics](https://analytics.google.com/) into the player
- Added support for integration three ad formats of [Google AdMob](https://www.google.com/admob/) into the player
- Added support for Unicode (UTF-8) almost everywhere
- Updated Bullet physics library to 2.88
- Fixed an issue where sRGB rendering on Android was incorrect
- Improved speed of project loading in editor

### 0.5.0
- Added support for GPU instancing for static/skinned meshes
- Added support for static batching
- Added support for calculation tangent vectors using MikkTSpace
- Added support for calculation normals with area and anglular weights
- Added support for vehicle physics
- Added support for slider/wheel joint components
- Added support for copy & paste component values in editor
- Changed engine unit from centi-meters to meters
- Changed FBX importing process to support hierarchical mesh in editor
- Fixed an issue where rolling friction was not working correctly
- Fixed an issue where inspector updating in play mode would slow down the game 

### 0.4.0
- Added support for LuaJIT (Windows / macOS)
- Added support for debugging Lua script in [ZeroBrane Studio](https://studio.zerobrane.com/)
- Reimplemented Android build from scratch
- Added support for Google reward based video ad (Android)
- Various minor improvements and bug fixes

### 0.3.0
- Improved property system
- Added support for editing multiple selected entities simultaneously
- Added support for undo/redo in play mode
- Added support for Google reward based video ad (iOS)
- Added component icons in editor for each component widgets
- New dock widgets
- Fixed a potential crash when using undo/redo

### 0.2.2
- Fixed crashes for Android player
- Various minor bug fixes

### 0.2.0
- Added support for physically based lighting
- Added support for particle system
- Improved material editor
- Added support for six sided skybox rendering
- Added support for conversion between equirectangular spherical image and cube image
- Various minor improvements and bug fixes

### 0.1.3
- Fixed bug to deploy target iOS/macOS

### 0.1.0
- First release