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

BE_NAMESPACE_BEGIN

template <typename T> 
struct Range {
    Range() : maxValue(0), minValue(0), step(0) {}
    Range(const T &minValue, const T &maxValue, const T &step);

    bool IsValid() const { return maxValue > minValue && step != 0; }

    const T &Clamp(const T &value) const;

    T minValue, maxValue, step;
};

using Rangei    = Range<int32_t>;
using Rangei64  = Range<int64_t>;
using Rangef    = Range<float>;
using Ranged    = Range<double>;

template <typename T> 
BE_INLINE Range<T>::Range(const T &minValue, const T &maxValue, const T &step) { 
    this->minValue = minValue; 
    this->maxValue = maxValue; 
    this->step = step; 
}

template <typename T>
BE_INLINE const T &Range<T>::Clamp(const T &value) const {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

BE_NAMESPACE_END
