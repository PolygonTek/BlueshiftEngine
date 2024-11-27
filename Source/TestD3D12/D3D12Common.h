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

#define USE_D3D12_MEMALLOC

#define USE_RENDER_THREAD

#define USE_RENDER_TASK

#ifdef USE_RENDER_TASK
// 렌더링할 때 쓰이는 최대 태스크 개수
static constexpr int            MaxRenderTasks = 64;
// 렌더링 태스크 매니져에서 사용할 최대 스레드 개수 (실제로는 물리 코어 개수가 최대로 사용하는 스레드 개수다)
static constexpr int            MaxRenderTaskThreads = 20;
#endif
