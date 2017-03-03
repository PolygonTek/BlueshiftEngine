#pragma once

#include "Core/Signal.h"
#include "Core/SignalObject.h"

BE_NAMESPACE_BEGIN

class CmdArgs;

#define	CONSOLE_TEXT_SIZE       0x40000
#define CONSOLE_NOTIFY_TIMES    4

extern const SignalDef          SIG_ConsoleOutputTextAdded;

class Console : public SignalObject {
public:
    Console() { initialized = false; }

    void                Init();
    void                Shutdown();

    const wchar_t *     GetText() const { return text; }

    void                CheckResize(int newSizeOfLine);

    void                Clear();
    void                ClearNotify();
    void                LineFeed();

    void                Print(const wchar_t *txt);

    void                DumpToFile(const char *filename);

    bool                initialized;

    wchar_t             text[CONSOLE_TEXT_SIZE];
    int                 sizeOfLine;
    int                 totalLines;
    int                 currentLine;
    int                 cursorPos;

    int                 notifyTimes[CONSOLE_NOTIFY_TIMES];

    static void         Cmd_ConClear(const CmdArgs &args);
    static void         Cmd_ConDump(const CmdArgs &args);
};

extern Console          console;

BE_NAMESPACE_END
