// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "BlueshiftEngine.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d11on12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <DirectXMath.h>

#if defined(_DEBUG) || defined(_DEVELOPMENT)
#define PROFILE_BUILD
#include <pix3.h>
#endif

using namespace BE1;
