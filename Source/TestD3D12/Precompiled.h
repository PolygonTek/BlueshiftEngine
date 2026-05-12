// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "BlueshiftEngine.h"

#ifdef USE_TASK_MANAGER
// 렌더 태스크 사용 여부
#define USE_RENDER_TASK
#endif

// 렌더 스레드 사용 여부
#define USE_RENDER_THREAD

// 프레임 버퍼링 사용 여부
#define USE_RENDER_FRAME_BUFFERING

// 인스턴싱 사용 여부
#define USE_RENDER_INSTANCED

// 렌더타겟 텍스쳐 사용 여부
//#define USE_OFFSCREEN_MAIN_RENDER_TARGET

// 프로파일러 사용 여부 (D3D12 의 경우엔 PIX, Release 모드에서는 무시됨)
#define USE_PROFILER

// 디버그 레이어 사용 여부 (Release 모드에서는 무시됨)
#define USE_RENDERER_DEBUG_LAYER

// 스레드 로딩 시스템 사용 여부
#define USE_ASYNC_LOADING

// DBVT 사용 여부
//#define USE_DBVT
