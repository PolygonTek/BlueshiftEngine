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
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

Console console;

const SignalDef Console::SIG_TextAdded("Console::TextAdded", "w");

void Console::Init() {
    cmdSystem.AddCommand(L"clear", Cmd_ConClear);
    cmdSystem.AddCommand(L"condump", Cmd_ConDump);

    CheckResize(0);

    ClearNotify();

    cursorPos = 0;

    initialized = true;
}

void Console::Shutdown() {
    cmdSystem.RemoveCommand(L"clear");
    cmdSystem.RemoveCommand(L"condump");

    initialized = false;
}

// size of line 이 바뀌었다면 버퍼를 재구성한다
void Console::CheckResize(int newSizeOfLine) {

    int width = newSizeOfLine / 7/*SYSTEM_FONT_WIDTH*/ - 2;

    if (width == sizeOfLine) {
        return;
    }
    //	wchar_t tbuf[CONSOLE_TEXT_SIZE];
    //wchar_t tbuf[CONSOLE_TEXT_SIZE];
    wchar_t *tbuf = new  wchar_t[CONSOLE_TEXT_SIZE];

    if (width < 1) { // initialize
        width = 1024 / 7/*SYSTEM_FONT_WIDTH*/ - 2;
        sizeOfLine = width;
        totalLines = CONSOLE_TEXT_SIZE / sizeOfLine;
        wmemset(text, L' ', CONSOLE_TEXT_SIZE);
    } else {
        int oldwidth = sizeOfLine;
        int oldTotalLines = totalLines;

        sizeOfLine = width;
        totalLines = CONSOLE_TEXT_SIZE / sizeOfLine;

        int numLines = Min(totalLines, oldTotalLines);
        int numChars = Min(sizeOfLine, oldwidth);

        wmemcpy(tbuf, text, CONSOLE_TEXT_SIZE);
        wmemset(text, L' ', CONSOLE_TEXT_SIZE);

        for (int i = 0; i < numLines; i++) {
            for (int j = 0; j < numChars; j++) {
                text[(totalLines - 1 - i) * sizeOfLine + j] = tbuf[((currentLine - i + oldTotalLines) % oldTotalLines) * oldwidth + j];
            }
        }
    }
    delete [] tbuf;

    currentLine = totalLines - 1;
}

void Console::Clear() {
    wmemset(text, L' ', COUNT_OF(text));
    currentLine = 0;
}

void Console::ClearNotify() {
    for (int i = 0; i < COUNT_OF(notifyTimes); i++) {
        notifyTimes[i] = 0;
    }
}

void Console::LineFeed() {
    cursorPos = 0;
    currentLine++;
    wmemset(&text[(currentLine % totalLines) * sizeOfLine], L' ', sizeOfLine); // 줄 청소
}

void Console::Print(const wchar_t *string) {
    wchar_t c;
    int len;

    if (!initialized) {
        return;
    }

    EmitSignal(&SIG_TextAdded, string);

    while ((c = *string)) {
        // count words
        for (len = 0; len < sizeOfLine; len++) {
            if (string[len] <= L' ') {
                if (string[len] != 0) {
                    len++;
                }
                break;
            }
        }

        // word wrap
        if (len < sizeOfLine && cursorPos + len > sizeOfLine) {
            cursorPos = 0;
        }

        do {
            c = *string++;

            if (!cursorPos) {
                LineFeed();
                if (currentLine >= 0) {
                    notifyTimes[currentLine % CONSOLE_NOTIFY_TIMES] = common.realTime;
                }
            }

            switch (c) {
            case L'\r': // ignore CR
                break;
            case L'\n':
                cursorPos = 0;
                break;
            case L'\t':
                cursorPos += 4;
                if (cursorPos >= sizeOfLine) {
                    cursorPos = sizeOfLine - 1;
                }
                break;
            default:
                text[(currentLine % totalLines) * sizeOfLine + cursorPos] = c;
                cursorPos++;
                if (cursorPos >= sizeOfLine) {
                    cursorPos = 0;
                }
                break;
            }
        } while (--len);
    }
}

void Console::DumpToFile(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_LOG(L"Console::DumpToFile: Couldn't open %hs.\n", filename);
        return;
    }

    int l, x;

    // skip empty command lines
    for (l = currentLine - totalLines + 1; l <= currentLine; l++) {
        wchar_t *line = text + (l % totalLines) * sizeOfLine;
        for (x = 0; x < sizeOfLine; x++) {
            if (line[x] != L' ') {
                break;
            }
        }

        if (x != sizeOfLine) {
            break;
        }
    }

    // write the remaining command lines
    wchar_t buffer[1024];
    buffer[sizeOfLine] = 0;
    for (; l <= currentLine; l++) {
        wchar_t *line = text + (l % totalLines) * sizeOfLine;
        WStr::Copynz(buffer, line, sizeOfLine);
        for (x = sizeOfLine - 2; x >= 0; x--) {
            if (buffer[x] == L' ') {
                buffer[x] = 0;
            } else {
                break;
            }
        }
        
        fp->Printf(L"%ls\n", buffer);
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
        BE_LOG(L"usage: condump <filename>\n");
        return;
    }

    Str::snPrintf(name, sizeof(name), "%ls.txt", args.Argv(1));

    console.DumpToFile(name);

    BE_LOG(L"Dumped console text to %hs.\n", name);
}

BE_NAMESPACE_END
