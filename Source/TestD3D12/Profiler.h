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

#include "RHI/RHI.h"

#if defined(USE_RHI_PROFILER) && defined(USE_TRACY_PROFILER)
#error USE_RHI_PROFILER and USE_TRACY_PROFILER are mutually exclusive for PROFILER_* aliases.
#endif

#if defined(USE_PROFILER) && (defined(_DEBUG) || defined(_DEVELOPMENT))

#define PROFILER_JOIN_IMPL(a, b) a##b
#define PROFILER_JOIN(a, b) PROFILER_JOIN_IMPL(a, b)

#if defined(USE_RHI_PROFILER)
#define RHI_PROFILER_STRING_TO_COLOR_INDEX(str) (fnv1a_hash(str) % 8)
#define RHI_PROFILER_CPU_BEGIN_EVENT(str) RHI::renderer->BeginEvent(nullptr, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#define RHI_PROFILER_CPU_END_EVENT() RHI::renderer->EndEvent(nullptr)
#define RHI_PROFILER_CPU_SCOPED_EVENT(str) RHI::ScopedProfilerEvent PROFILER_JOIN(_rhiProfilerScopedEvent, __LINE__)(nullptr, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#define RHI_PROFILER_CPU_MARKER(str) RHI::renderer->SetMarker(nullptr, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#define RHI_PROFILER_BEGIN_EVENT(commandList, str) RHI::renderer->BeginEvent(commandList, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#define RHI_PROFILER_END_EVENT(commandList) RHI::renderer->EndEvent(commandList)
#define RHI_PROFILER_SCOPED_EVENT(commandList, str) RHI::ScopedProfilerEvent PROFILER_JOIN(_rhiProfilerScopedEvent, __LINE__)(commandList, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#define RHI_PROFILER_MARKER(commandList, str) RHI::renderer->SetMarker(commandList, str, RHI_PROFILER_STRING_TO_COLOR_INDEX(str))
#elif defined(USE_TRACY_PROFILER)
#include <tracy/Tracy.hpp>
#define TRACY_PROFILER_CPU_BEGIN_EVENT(str)
#define TRACY_PROFILER_CPU_END_EVENT()
#define TRACY_PROFILER_CPU_SCOPED_EVENT(str) ZoneNamedN(PROFILER_JOIN(_tracyProfilerCpuScopedEvent, __LINE__), str, true)
#define TRACY_PROFILER_CPU_MARKER(str) TracyMessageL(str)
#define TRACY_PROFILER_BEGIN_EVENT(commandList, str)
#define TRACY_PROFILER_END_EVENT(commandList)
#define TRACY_PROFILER_SCOPED_EVENT(commandList, str)
#define TRACY_PROFILER_MARKER(commandList, str)
#define TRACY_PROFILER_FRAME_MARK() FrameMark
#endif

#if defined(USE_RHI_PROFILER)
#define PROFILER_INIT()
#define PROFILER_SHUTDOWN()
#define PROFILER_CPU_BEGIN_EVENT(str) RHI_PROFILER_CPU_BEGIN_EVENT(str)
#define PROFILER_CPU_END_EVENT() RHI_PROFILER_CPU_END_EVENT()
#define PROFILER_CPU_SCOPED_EVENT(str) RHI_PROFILER_CPU_SCOPED_EVENT(str)
#define PROFILER_CPU_MARKER(str) RHI_PROFILER_CPU_MARKER(str)
#define PROFILER_BEGIN_EVENT(commandList, str) RHI_PROFILER_BEGIN_EVENT(commandList, str)
#define PROFILER_END_EVENT(commandList) RHI_PROFILER_END_EVENT(commandList)
#define PROFILER_SCOPED_EVENT(commandList, str) RHI_PROFILER_SCOPED_EVENT(commandList, str)
#define PROFILER_MARKER(commandList, str) RHI_PROFILER_MARKER(commandList, str)
#define PROFILER_FRAME_MARK()
#elif defined(USE_TRACY_PROFILER)
#define PROFILER_INIT() tracy::StartupProfiler()
#define PROFILER_SHUTDOWN() tracy::ShutdownProfiler()
#define PROFILER_CPU_BEGIN_EVENT(str) TRACY_PROFILER_CPU_BEGIN_EVENT(str)
#define PROFILER_CPU_END_EVENT() TRACY_PROFILER_CPU_END_EVENT()
#define PROFILER_CPU_SCOPED_EVENT(str) TRACY_PROFILER_CPU_SCOPED_EVENT(str)
#define PROFILER_CPU_MARKER(str) TRACY_PROFILER_CPU_MARKER(str)
#define PROFILER_BEGIN_EVENT(commandList, str) TRACY_PROFILER_BEGIN_EVENT(commandList, str)
#define PROFILER_END_EVENT(commandList) TRACY_PROFILER_END_EVENT(commandList)
#define PROFILER_SCOPED_EVENT(commandList, str) TRACY_PROFILER_SCOPED_EVENT(commandList, str)
#define PROFILER_MARKER(commandList, str) TRACY_PROFILER_MARKER(commandList, str)
#define PROFILER_FRAME_MARK() TRACY_PROFILER_FRAME_MARK()
#else
#define PROFILER_INIT()
#define PROFILER_SHUTDOWN()
#define PROFILER_CPU_BEGIN_EVENT(str)
#define PROFILER_CPU_END_EVENT()
#define PROFILER_CPU_SCOPED_EVENT(str)
#define PROFILER_CPU_MARKER(str)
#define PROFILER_BEGIN_EVENT(commandList, str)
#define PROFILER_END_EVENT(commandList)
#define PROFILER_SCOPED_EVENT(commandList, str)
#define PROFILER_MARKER(commandList, str)
#define PROFILER_FRAME_MARK()
#endif

#else

#define PROFILER_INIT()
#define PROFILER_SHUTDOWN()
#define PROFILER_CPU_BEGIN_EVENT(str)
#define PROFILER_CPU_END_EVENT()
#define PROFILER_CPU_SCOPED_EVENT(str)
#define PROFILER_CPU_MARKER(str)
#define PROFILER_BEGIN_EVENT(commandList, str)
#define PROFILER_END_EVENT(commandList)
#define PROFILER_SCOPED_EVENT(commandList, str)
#define PROFILER_MARKER(commandList, str)
#define PROFILER_FRAME_MARK()

#endif
