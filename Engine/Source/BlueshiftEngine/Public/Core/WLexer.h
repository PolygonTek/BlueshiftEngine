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

#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

class BE_API WLexer {
public:
    struct Punctuation {
        const wchar_t * p;      ///< punctuation character(s)
        PuncType        n;      ///< punctuation id
    };

    WLexer(int flags = 0);
    WLexer(const wchar_t *text, int size, const char *name, int flags = 0);
    ~WLexer();

    void                Init(int flags);
    void                Free();

    void                SetPunctuations(const Punctuation *punctuations);

    bool                Load(const wchar_t *text, int size, const char *name, int startLine = 0);

    bool                EndOfFile() const { return ((ptr >= endPtr) ? true : false); }
    int                 GetCurrentOffset() const { return (int)(ptr - buffer); }

    void                Warning(const wchar_t *fmt, ...);
    void                Error(const wchar_t *fmt, ...);
    const char *        GetLastErrorMessage() const { return errorMessage; }
    
    bool                ReadToken(WStr *token, bool allowLineBreaks = true);
    void                UnreadToken(const WStr *token);
    int                 GetTokenType() const { return tokenType; }
    int                 GetPunctuationType() const { return punctuationType; }
    int                 LinesCrossed() const { return linesCrossed; }

    int                 CheckTokenString(const wchar_t *string, bool allowLineBreaks = true);

    float               ParseNumber();
    int                 ParseInt();
    float               ParseFloat();
    bool                ParseVec(int num, float *v);
    int                 Parse1DMatrix(int x, float *m);
    int                 Parse2DMatrix(int y, int x, float *m);
    int                 Parse3DMatrix(int z, int y, int x, float *m);
    const wchar_t *     ParseBracedSectionExact(WStr &out, int tabs = -1);
    const wchar_t *     ParseBracedSection(WStr &out);
    
    bool                ExpectTokenType(int tokenType, WStr *token);
    bool                ExpectTokenString(const wchar_t *string, bool caseSensitive = true);
    bool                ExpectPunctuation(int type);

    void                SkipWhitespace();
    void                SkipRestOfLine();
    void                SkipBracedSection(bool parseFirstBrace = true);
    bool                SkipUntilString(const wchar_t *string);

    int                 WhiteSpaceBeforeToken() const;

private:
    bool                ReadString(WStr *token, wchar_t quote);
    bool                ReadNumber(WStr *token);
    bool                ReadIdentifier(WStr *token);
    bool                ReadPunctuation(WStr *token);

    const wchar_t *     GetPunctuationString(int type) const;

    char                filename[MaxRelativePath];
    char                errorMessage[256];
    const wchar_t *     buffer;
    int                 size;
    const wchar_t *     ptr;
    const wchar_t *     lastPtr;
    const wchar_t *     endPtr;
    int                 line;
    int                 linesCrossed;
    const wchar_t *     whiteSpaceBegin_p;
    const wchar_t *     whiteSpaceEnd_p;
    bool                loaded;
    bool                alloced;
    int                 tokenType;
    int                 punctuationType;
    WStr                token;
    bool                tokenAvailable;
    const Punctuation * punctuations;
    int                 flags;
};

BE_INLINE WLexer::WLexer(int flags) {
    Init(flags);
}

BE_INLINE WLexer::WLexer(const wchar_t *text, int size, const char *name, int flags) {
    Init(flags);
    Load(text, size, name, 0);
}

BE_INLINE WLexer::~WLexer() {
    Free();
}

BE_INLINE int WLexer::WhiteSpaceBeforeToken() const {
    return (whiteSpaceEnd_p > whiteSpaceBegin_p);
}

BE_NAMESPACE_END
