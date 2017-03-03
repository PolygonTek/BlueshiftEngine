#pragma once

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class _FileMapping {
public:
    _FileMapping() {
        size = 0;
        data = NULL;
    }
    void            Open(const void *_data, size_t _size) {
        data = _data;
        size = _size;
    }
    void            Touch();

    const char *    GetData() const { return (const char *)data; }
    size_t          GetSize() const { return size; }

protected:
    size_t          size;
    const void *    data;
};

class FileMapping : public _FileMapping
{
public:
    FileMapping();
    FileMapping & operator = (const FileMapping &value);
    bool            Open(const TCHAR *filename);
    bool            Open(const Str &filename);
    void            Close();
private:
    Str            filename;
#if defined(__WIN32__)
    HANDLE          hFile;
    HANDLE          hMapping;
#elif defined __UNIX__
    int             hFile;
#endif
};

BE_INLINE FileMapping & FileMapping::operator = (const FileMapping &value) 
{
//	*(_FileMapping *) this = (_FileMapping &)value;
#ifdef __WIN32__
    hFile = value.hFile;
    hMapping = value.hMapping;
#elif defined __UNIX__
    hFile = value.hFile;
#endif
    return *this;
}

BE_NAMESPACE_END
