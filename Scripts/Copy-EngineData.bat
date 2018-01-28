@pushd %~dp0
@if exist "..\Engine\Bin" rd "..\Engine\Bin" /s /q
@xcopy "..\EngineData\Bin" "..\Engine\Bin" /I /e /y
@if exist "..\Engine\Data" rd "..\Engine\Data" /s /q
@xcopy "..\EngineData\Data" "..\Engine\Data" /I /e /y
@if exist "..\Engine\Tools" rd "..\Engine\Tools" /s /q
@xcopy "..\EngineData\Tools\PVRTexTool\Windows_x86_64" "..\Engine\Tools\PVRTexTool\Windows_x86_64" /I /e /y
@popd 
@pause
