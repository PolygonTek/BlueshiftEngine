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

#include <cstdint>
#include <type_traits>
#include <utility>
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Scoped CPU Timer

-------------------------------------------------------------------------------
*/

namespace detail {
    struct CpuTimerNoCallback {
        void operator()(uint64_t) const noexcept {}
    };
}

template <typename Callback = detail::CpuTimerNoCallback>
class ScopedCpuTimer {
public:
    static_assert(std::is_nothrow_invocable_v<Callback &, const uint64_t &>, "ScopedCpuTimer callback must accept elapsed microseconds and be noexcept");

    // Skip clock reads, accumulation, and callback invocation when disabled.
    explicit ScopedCpuTimer(uint64_t &accumulatedUsec, bool enabled = true, Callback callback = {}) :
        callback(std::move(callback)),
        accumulatedUsec(enabled ? &accumulatedUsec : nullptr),
        startUsec(enabled ? PlatformTime::Microseconds() : 0) {
    }

    ~ScopedCpuTimer() noexcept {
        if (accumulatedUsec) {
            const uint64_t elapsedUsec = PlatformTime::Microseconds() - startUsec;
            *accumulatedUsec += elapsedUsec;

            // Pass this scope's elapsed time after accumulation. Callback execution time is excluded.
            callback(elapsedUsec);
        }
    }

    ScopedCpuTimer(const ScopedCpuTimer &) = delete;
    ScopedCpuTimer &operator=(const ScopedCpuTimer &) = delete;
    ScopedCpuTimer(ScopedCpuTimer &&) = delete;
    ScopedCpuTimer &operator=(ScopedCpuTimer &&) = delete;

private:
    Callback            callback;
    uint64_t *          accumulatedUsec;
    uint64_t            startUsec;
};

BE_NAMESPACE_END
