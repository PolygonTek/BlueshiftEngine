// ConsoleDll.cpp : Defines the exported functions for the DLL application.
//

#include "PreCompiled.h"
//#include <mono/jit/jit.h>
//#include <mono/metadata/object.h>
//#include <mono/metadata/environment.h>
//#include <mono/metadata/assembly.h>
//#include <mono/metadata/debug-helpers.h>
#include <string.h>
#include <stdlib.h>
#include "Client.h"

extern "C"  CLIENT_EXPORT void CLIENT_API SendFrame()
{
	SendPacket(C2S_FRAME, "");
}

extern "C"  CLIENT_EXPORT void CLIENT_API SendSound2D(char *name)
{
	SendPacket(C2S_SOUND2D, "s", name);
}
