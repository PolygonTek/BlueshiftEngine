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

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// Lexer flags
enum LexerFlag {
    LEXFL_NOWARNINGS            = BIT(0),   // don't print any warnings
    LEXFL_NOERRORS              = BIT(1),   // don't print any errors
    LEXFL_ALLOW_FLOAT_NAN       = BIT(2),
    LEXFL_IGNORE_COMMENTS       = BIT(3)
};

/// Token type
enum TokenType {
    TT_PUNCTUATION              = BIT(0),
    TT_IDENTIFIER               = BIT(1),
    TT_STRING                   = BIT(2),
    TT_LITERAL                  = BIT(3),
    TT_NUMBER                   = BIT(4),
    TT_INTEGER                  = BIT(5),   // TT_NUMBER
    TT_UNSIGNED                 = BIT(6),   // TT_NUMBER | TT_INTEGER
    TT_LONG                     = BIT(7),   // TT_NUMBER | TT_INTEGER
    TT_DECIMAL                  = BIT(8),   // TT_NUMBER | TT_INTEGER 
    TT_OCTAL                    = BIT(9),   // TT_NUMBER | TT_INTEGER
    TT_HEXADECIMAL              = BIT(10),  // TT_NUMBER | TT_INTEGER
    TT_FLOAT                    = BIT(11),  // TT_NUMBER
    TT_SINGLE_PRECISION         = BIT(12),  // TT_NUMBER | TT_FLOAT
    TT_DOUBLE_PRECISION         = BIT(13),  // TT_NUMBER | TT_FLOAT
    TT_EXTENDED_PRECISION       = BIT(14),  // TT_NUMBER | TT_FLOAT
    TT_NAN                      = BIT(15),  // TT_NUMBER | TT_FLOAT
    TT_INFINITE                 = BIT(16),  // TT_NUMBER | TT_FLOAT | TT_NAN
    TT_INDEFINITE               = BIT(17)   // TT_NUMBER | TT_FLOAT | TT_NAN
};

/// Punctuation type
enum PuncType {
    P_NONE                      = 0,    //
    P_RSHIFT_ASSIGN             = 1,    // >>=
    P_LSHIFT_ASSIGN             = 2,    // <<=
    P_PARMS                     = 3,    // ...
    P_PRECOMPMERGE              = 4,    // ##

    P_LOGIC_AND                 = 5,    // &&
    P_LOGIC_OR                  = 6,    // ||
    P_LOGIC_GEQ                 = 7,    // >=
    P_LOGIC_LEQ                 = 8,    // <=
    P_LOGIC_EQ                  = 9,    // ==
    P_LOGIC_UNEQ                = 10,   // !=

    P_MUL_ASSIGN                = 11,   // *=
    P_DIV_ASSIGN                = 12,   // /=
    P_MOD_ASSIGN                = 13,   // %=
    P_ADD_ASSIGN                = 14,   // +=
    P_SUB_ASSIGN                = 15,   // -=
    P_INC                       = 16,   // ++
    P_DEC                       = 17,   // --

    P_BIN_AND_ASSIGN            = 18,   // &=
    P_BIN_OR_ASSIGN             = 19,   // |=
    P_BIN_XOR_ASSIGN            = 20,   // ^=
    P_RSHIFT                    = 21,   // >>
    P_LSHIFT                    = 22,   // <<

    P_POINTERREF                = 23,   // ->
    P_CPP1                      = 24,   // ::
    P_CPP2                      = 25,   // .*
    P_MUL                       = 26,   // *
    P_DIV                       = 27,   // /
    P_MOD                       = 28,   // %
    P_ADD                       = 29,   // +
    P_SUB                       = 30,   // -
    P_ASSIGN                    = 31,   // =

    P_BIN_AND                   = 32,   // &
    P_BIN_OR                    = 33,   // |
    P_BIN_XOR                   = 34,   // ^
    P_BIN_NOT                   = 35,   // ~

    P_LOGIC_NOT                 = 36,   // !
    P_LOGIC_GREATER             = 37,   // >
    P_LOGIC_LESS                = 38,   // <

    P_REF                       = 39,   // .
    P_COMMA                     = 40,   // ,
    P_SEMICOLON                 = 41,   // ;
    P_COLON                     = 42,   // :
    P_QUESTIONMARK              = 43,   // ?

    P_PARENTHESESOPEN           = 44,   // (
    P_PARENTHESESCLOSE          = 45,   // )
    P_BRACEOPEN                 = 46,   // {
    P_BRACECLOSE                = 47,   // }
    P_SQBRACKETOPEN             = 48,   // [ 
    P_SQBRACKETCLOSE            = 49,   // ]
    P_BACKSLASH                 = 50,   // '\'

    P_PRECOMP                   = 51,   // #
    P_DOLLAR                    = 52    // $
};

class BE_API Lexer {
public:
    struct Punctuation {
        const char *    p;      ///< punctuation character(s)
        PuncType        n;      ///< punctuation id
    };

    Lexer(int flags = 0);
    Lexer(const char *text, int size, const char *name, int flags = 0);
    ~Lexer();

    void                Init(int flags);
    void                Free();

    void                SetPunctuations(const Punctuation *punctuations);

    bool                Load(const char *text, int size, const char *name, int startLine = 0);

    bool                EndOfFile() const { return ((ptr >= endPtr) ? true : false); }
    int                 GetCurrentOffset() const { return (int)(ptr - buffer); }

    void                Warning(const char *fmt, ...);
    void                Error(const char *fmt, ...);
    const char *        GetLastErrorMessage() const { return errorMessage; }
    
    bool                ReadToken(Str *token, bool allowLineBreaks = true);
    void                UnreadToken(const Str *token);
    int                 GetTokenType() const { return tokenType; }
    int                 GetPunctuationType() const { return punctuationType; }
    int                 LinesCrossed() const { return linesCrossed; }

    int                 CheckTokenString(const char *string, bool allowLineBreaks = true);

    float               ParseNumber();
    int                 ParseInt();
    float               ParseFloat();
    bool                ParseVec(int num, float *v);
    int                 Parse1DMatrix(int x, float *m);
    int                 Parse2DMatrix(int y, int x, float *m);
    int                 Parse3DMatrix(int z, int y, int x, float *m);
    const char *        ParseBracedSectionExact(Str &out, int tabs = -1);
    const char *        ParseBracedSection(Str &out);
    
    bool                ExpectTokenType(int tokenType, Str *token);
    bool                ExpectTokenString(const char *string, bool caseSensitive = true);
    bool                ExpectPunctuation(int type);

    void                SkipWhitespace();
    void                SkipRestOfLine();
    void                SkipBracedSection(bool parseFirstBrace = true);
    bool                SkipUntilString(const char *string);

    int                 WhiteSpaceBeforeToken() const;

private:
    bool                ReadString(Str *token, int quote);
    bool                ReadNumber(Str *token);
    bool                ReadIdentifier(Str *token);
    bool                ReadPunctuation(Str *token);

    const char *        GetPunctuationString(int type) const;

    char                filename[MaxRelativePath];
    char                errorMessage[256];
    const char *        buffer;
    int                 size;
    const char *        ptr;
    const char *        lastPtr;
    const char *        endPtr;
    int                 line;
    int                 linesCrossed;
    const char *        whiteSpaceBegin_p;
    const char *        whiteSpaceEnd_p;
    bool                loaded;
    bool                alloced;
    int                 tokenType;
    int                 punctuationType;
    Str                 token;
    bool                tokenAvailable;
    const Punctuation * punctuations;
    int                 flags;
};

BE_INLINE Lexer::Lexer(int flags) {
    Init(flags);
}

BE_INLINE Lexer::Lexer(const char *text, int size, const char *name, int flags) {
    Init(flags);
    Load(text, size, name, 0);
}

BE_INLINE Lexer::~Lexer() {
    Free();
}

BE_INLINE int Lexer::WhiteSpaceBeforeToken() const {
    return (whiteSpaceEnd_p > whiteSpaceBegin_p);
}

BE_NAMESPACE_END
