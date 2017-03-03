#pragma once

/*
-------------------------------------------------------------------------------

    Binary Search templated functions

    The array elements have to be ordered in increasing order.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Finds the element which is equal to the given value.
template <typename T> int BinSearch_Equal(const T *arr, const int arraySize, const T &value) {
    int len = arraySize;
    int mid = len;
    int offset = 0;

    while (mid > 0) {
        mid = len >> 1;
        if (value == arr[offset + mid]) {
            return offset + mid;
        } else if (value > arr[offset + mid]) {
            offset += mid;
        }
        len -= mid;
    }

    return -1;
}

/// Finds the last array element which is smaller than the given value.
template <typename T> int BinSearch_Less(const T *arr, const int arraySize, const T &value) {
    int len = arraySize;
    int mid = len;
    int offset = 0;

    while (mid > 0) {
        mid = len >> 1;
        if (value > arr[offset + mid]) {
            offset += mid;
        }
        len -= mid;
    }

    return offset;
}

/// Finds the last array element which is smaller than or equal to the given value.
template <typename T> int BinSearch_LessEqual(const T *arr, const int arraySize, const T &value) {
    int len = arraySize;
    int mid = len;
    int offset = 0;

    while (mid > 0) {
        mid = len >> 1;
        if (value >= arr[offset + mid]) {
            offset += mid;
        }
        len -= mid;
    }

    return offset;
}

/// Finds the first array element which is greater than the given value.
template <typename T> int BinSearch_Greater(const T *arr, const int arraySize, const T &value) {
    int len = arraySize;
    int mid = len;
    int offset = 0;
    int res = 0;

    while (mid > 0) {
        mid = len >> 1;
        if (value < arr[offset + mid]) {
            res = 0;
        } else {
            offset += mid;
            res = 1;
        }
        len -= mid;
    }

    return offset + res;
}

/// Finds the first array element which is greater or equal than the given value.
template <typename T> int BinSearch_GreaterEqual(const T *arr, const int arraySize, const T &value) {
    int len = arraySize;
    int mid = len;
    int offset = 0;
    int res = 0;

    while (mid > 0) {
        mid = len >> 1;
        if (value <= arr[offset + mid]) {
            res = 0;
        } else {
            offset += mid;
            res = 1;
        }
        len -= mid;
    }

    return offset + res;
}

BE_NAMESPACE_END
