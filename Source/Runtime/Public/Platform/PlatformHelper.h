#pragma once

B_NAMESPACE_BEGIN

// type for shared library
typedef struct opaque_lib_t *lib_t;

const char * B_API GetPlatformName();
const char * B_API GetExecutableFileName();
int B_API GetNumberOfLogicalProcessors();

// loads a shared library
lib_t B_API OpenLibrary(const char *filename);
// unloads a shared library
void B_API CloseLibrary(lib_t lib);
// returns address of a symbol from the library
void * B_API GetSymbol(lib_t lib, const char *sym);

#ifdef __WIN32__
WStr B_API GetLastErrorText();
#endif

B_NAMESPACE_END
