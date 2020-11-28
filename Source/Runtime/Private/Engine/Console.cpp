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

#include "Precompiled.h"
#include "Engine/Common.h"
#include "Engine/Console.h"
#include "Core/Signal.h"
#include "Core/Cmds.h"
#include "IO/FileSystem.h"

BE_NAMESPACE_BEGIN

Console console;

const SignalDef Console::SIG_TextAdded("Console::TextAdded", "s");

void Console::Init() {
    cmdSystem.AddCommand("clear", Cmd_ConClear);
    cmdSystem.AddCommand("condump", Cmd_ConDump);

    textLines.SetCount(4096);

    currentLineIndex = 0;

    initialized = true;
}

void Console::Shutdown() {
    cmdSystem.RemoveCommand("clear");
    cmdSystem.RemoveCommand("condump");

    initialized = false;
}

void Console::Clear() {
    for (int i = 0; i < textLines.Count(); i++) {
        textLines[i].Clear();
    }

    currentLineIndex = 0;
}

int Console::GetFirstLineIndex() const {
    int index = (currentLineIndex + 1) % textLines.Count();
    while (index != currentLineIndex) {
        if (!textLines[index].IsEmpty()) {
            break;
        }
        index = (index + 1) % textLines.Count();
    }

    return index;
}

int Console::NumLines() const {
    int numLines = console.currentLineIndex - console.GetFirstLineIndex();
    if (numLines < 0) {
        numLines += console.textLines.Count();
    }
    return numLines + 1;
}

void Console::Print(const Str &inString) {
    if (!initialized) {
        return;
    }

    EmitSignal(&SIG_TextAdded, inString);

    int offset = 0;
    char32_t unicodeChar;

    while ((unicodeChar = inString.UTF8CharAdvance(offset))) {
        switch (unicodeChar) {
        case U'\t':
            textLines[currentLineIndex].Append("    ");
            break;
        case U'\r': // ignore CR
            break;
        case U'\n':
            currentLineIndex = (currentLineIndex + 1) % textLines.Count();
            textLines[currentLineIndex].Clear();
            break;
        default:
            textLines[currentLineIndex].AppendUTF8Char(unicodeChar);
            break;
        }
    }
}

void Console::DumpToFile(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_LOG("Console::DumpToFile: Couldn't open %s.\n", filename);
        return;
    }

    int firstLineIndex = GetFirstLineIndex();

    for (int i = firstLineIndex; i != currentLineIndex; i = (i + 1) % textLines.Count()) {
        fp->Printf("%s\n", textLines[i].c_str());
    }

    fileSystem.CloseFile(fp);
}

//--------------------------------------------------------------------------------------------------

void Console::Cmd_ConClear(const CmdArgs &args) {
    console.Clear();
}

void Console::Cmd_ConDump(const CmdArgs &args) {
    char name[256];

    if (args.Argc() != 2) {
        BE_LOG("usage: condump <filename>\n");
        return;
    }

    Str::snPrintf(name, sizeof(name), "%s.txt", args.Argv(1));

    console.DumpToFile(name);

    BE_LOG("Dumped console text to %s.\n", name);
}

BE_NAMESPACE_END
