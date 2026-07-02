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

#include <memory>
#include <utility>

#include "BlueshiftEngine.h"

// 프로파일러 사용 여부 (Release 모드에서는 무시됨)
#define USE_PROFILER

// 기존 PROFILER_* 매크로가 사용할 백엔드 선택
//#define USE_RHI_PROFILER
#define USE_TRACY_PROFILER

#ifdef USE_TASK_MANAGER
// 렌더 태스크 사용 여부
#define USE_RENDER_TASK
#endif

// 렌더 스레드 사용 여부
#define USE_RENDER_THREAD

// 프레임 버퍼링 사용 여부
#define USE_RENDER_FRAME_BUFFERING

// 스레드 로딩 시스템 사용 여부
#define USE_ASYNC_LOADING

// 텍스쳐 밉맵 스트리밍 시스템 사용 여부
#define USE_MIPMAP_STREAMING

// DBVT 사용 여부
//#define USE_DBVT

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T, typename... Args>
BE_INLINE SharedPtr<T> MakeShared(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
BE_INLINE UniquePtr<T> MakeUnique(Args &&...args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#include "Profiler.h"
