#pragma once

BE_NAMESPACE_BEGIN

template <typename T> 
struct Range {
    Range() {}
    Range(const T &minValue, const T &maxValue, const T &step); 

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
