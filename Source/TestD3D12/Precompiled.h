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

// 렌더 스레드 사용 여부
#define USE_RENDER_THREAD

// 프레임 리소스 버퍼링 사용 여부
#define USE_RENDER_FRAME_RESOURCES

// 인스턴싱 사용 여부
#define USE_RENDEROBJECT_INSTANCING

// 프로파일러 사용 여부 (D3D12 의 경우엔 PIX, Release 모드에서는 무시됨)
#define USE_PROFILER

// 디버그 레이어 사용 여부 (Release 모드에서는 무시됨)
#define USE_RENDERER_DEBUG_LAYER
