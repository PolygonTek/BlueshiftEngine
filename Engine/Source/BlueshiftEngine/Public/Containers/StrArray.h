#pragma once

/*
-------------------------------------------------------------------------------

    StrArray

-------------------------------------------------------------------------------
*/

#include "Array.h"
#include "Core/Str.h"
#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

using StrArray = Array<Str>;

BE_INLINE void SplitStringIntoList(StrArray &array, const char *string, const char *separator = "|") {
    int separatorLength = Str::Length(separator);

    assert(separatorLength > 0);

    Str str(string);

    // append a terminator there's no terminating one
    if (Str::Icmp(str.Mid(str.Length() - separatorLength, separatorLength), separator) != 0) {
        str += separator;
    }

    int startIndex = 0;
    int endIndex = str.Find(separator);

    while (endIndex != -1 && endIndex < str.Length()) {
        array.Append(str.Mid(startIndex, endIndex - startIndex));
        startIndex = endIndex + separatorLength;
        endIndex = str.Find(separator, false, startIndex);
    }
}

template <>
BE_INLINE size_t StrArray::Size() const {
    size_t s = sizeof(*this);
    for (int i = 0; i < Count(); i++) {
        s += (*this)[i].Size();
    }

    return s;
}

/*
-------------------------------------------------------------------------------

    WStrArray

-------------------------------------------------------------------------------
*/

using WStrArray = Array<WStr>;

BE_INLINE void SplitStringIntoList(WStrArray &array, const wchar_t *string, const wchar_t *separator = L"|") {
    int separatorLength = WStr::Length(separator);

    assert(separatorLength > 0);

    WStr str(string);

    // append a terminator there's no terminating one
    if (WStr::Icmp(str.Mid(str.Length() - separatorLength, separatorLength).c_str(), separator) != 0) {
        str += separator;
    }

    int startIndex = 0;
    int endIndex = str.Find(separator);

    while (endIndex != -1 && endIndex < str.Length()) {
        array.Append(str.Mid(startIndex, endIndex - startIndex));
        startIndex = endIndex + separatorLength;
        endIndex = str.Find(separator, false, startIndex);
    }
}

template <>
BE_INLINE size_t WStrArray::Size() const {
    size_t s = sizeof(*this);
    for (int i = 0; i < Count(); i++) {
        s += (*this)[i].Size();
    }

    return s;
}

BE_NAMESPACE_END
