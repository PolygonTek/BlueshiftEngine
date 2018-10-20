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

#include "Core/Signal.h"
#include "Core/SignalObject.h"
#include "Containers/StrArray.h"

BE_NAMESPACE_BEGIN

class CmdArgs;

#define CONSOLE_NOTIFY_TIMES    4

class Console : public SignalObject {
public:
    Console() { initialized = false; }

    void                Init();
    void                Shutdown();

    void                Clear();

    int                 GetFirstLineIndex() const;
    int                 NumLines() const;

    void                Print(const Str &string);

    void                DumpToFile(const char *filename);

    bool                initialized;

    StrArray            textLines;
    int                 currentLineIndex;

    static void         Cmd_ConClear(const CmdArgs &args);
    static void         Cmd_ConDump(const CmdArgs &args);

    static const SignalDef SIG_TextAdded;
};

extern Console          console;

BE_NAMESPACE_END
