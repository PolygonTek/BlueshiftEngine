nuget.exe install Microsoft.Direct3D.D3D12 -Source https://api.nuget.org/v3/index.json -OutputDirectory D3D12
nuget.exe install WinPixEventRuntime -OutputDirectory PIX
pause
:: Please manually copy the files listed below for Microsoft.Direct3D12.D3D12.
::
:: copy build/native/include/* to Source/ThirdParty/DirectX12/include
:: copy build/native/src/* to Source/ThirdParty/DirectX12/src
:: copy build/native/bin/x64/D3D12Core.dll to Bin/Win64/Debug, Bin/Win64/Release, Bin/Win64/Development
:: copy build/native/bin/x64/D3D12Core.pdb to Bin/Win64/Debug, Bin/Win64/Development
:: copy build/native/bin/x64/D3D12SDKLayers.dll to Bin/Win64/Debug, Bin/Win64/Release, Bin/Win64/Development
:: copy build/native/bin/x64/D3D12SDKLayers.pdb to Bin/Win64/Debug, Bin/Win64/Development

:: Please manually copy the files listed below for WinPixEventRuntime.
::
:: copy include/WinPixEventRuntime/* to Source/ThirdParty/PIX/include
:: copy bin/x64/WinPixEventRuntime.lib to Source/ThirdParty/PIX/lib
:: copy bin/x64/WinPixEventRuntime.dll to Bin/Win64/Debug, Bin/Win64/Release, Bin/Win64/Development
