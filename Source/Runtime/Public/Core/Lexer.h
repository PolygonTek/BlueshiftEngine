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

class BE_API Lexer {
public:
    /// Lexer flags
    struct Flag {
        enum Enum {
            NoWarnings          = BIT(0),   // don't print any warnings
            NoErrors            = BIT(1),   // don't print any errors
            AllowFloatNan       = BIT(2),
            IgnoreComments      = BIT(3)
        };
    };

    /// Token type
    struct TokenType {
        enum Enum {
            Punctuation         = BIT(0),
            Identifier          = BIT(1),
            String              = BIT(2),
            Literal             = BIT(3),
            Number              = BIT(4),
            Integer             = BIT(5),   // Number
            Unsigned            = BIT(6),   // Number | Integer
            Long                = BIT(7),   // Number | Integer
            Decimal             = BIT(8),   // Number | Integer 
            Octal               = BIT(9),   // Number | Integer
            HexaDecimal         = BIT(10),  // Number | Integer
            Float               = BIT(11),  // Number
            SinglePrecision     = BIT(12),  // Number | Float
            DoublePrecision     = BIT(13),  // Number | Float
            ExtendedPrecision   = BIT(14),  // Number | Float
            Nan                 = BIT(15),  // Number | Float
            Infinite            = BIT(16),  // Number | Float | Nan
            Indefinite          = BIT(17)   // Number | Float | Nan
        };
    };

    /// Punctuation type
    struct PuncType {
        enum Enum {
            None                = 0,    //
            RShiftAssign        = 1,    // >>=
            LShiftAssign        = 2,    // <<=
            Parms               = 3,    // ...
            PrecompMerge        = 4,    // ##

            LogicAnd            = 5,    // &&
            LogicOr             = 6,    // ||
            LogicGreaterEq      = 7,    // >=
            LogicLessEqual      = 8,    // <=
            LogicEqual          = 9,    // ==
            LogicUnequal        = 10,   // !=

            MulAssign           = 11,   // *=
            DivAssign           = 12,   // /=
            ModAssign           = 13,   // %=
            AddAssign           = 14,   // +=
            SubAssign           = 15,   // -=
            Inc                 = 16,   // ++
            Dec                 = 17,   // --

            BinAndAssign        = 18,   // &=
            BinOrAssign         = 19,   // |=
            BinXorAssign        = 20,   // ^=
            RShift              = 21,   // >>
            LShift              = 22,   // <<

            PointerRef          = 23,   // ->
            Cpp1                = 24,   // ::
            Cpp2                = 25,   // .*
            Mul                 = 26,   // *
            Div                 = 27,   // /
            Mod                 = 28,   // %
            Add                 = 29,   // +
            Sub                 = 30,   // -
            Assign              = 31,   // =

            BinAnd              = 32,   // &
            BinOr               = 33,   // |
            BinXor              = 34,   // ^
            BinNot              = 35,   // ~

            LogicNot            = 36,   // !
            LogicGreater        = 37,   // >
            LogicLess           = 38,   // <

            Ref                 = 39,   // .
            Comma               = 40,   // ,
            SemiColon           = 41,   // ;
            Colon               = 42,   // :
            QuestionMark        = 43,   // ?

            ParenthesesOpen     = 44,   // (
            ParenthesesClose    = 45,   // )
            BraceOpen           = 46,   // {
            BraceClose          = 47,   // }
            SquareBracketOpen   = 48,   // [ 
            SquareBracketClose  = 49,   // ]
            BackSlash           = 50,   // '\'

            Precomp             = 51,   // #
            Dollar              = 52    // $
        };
    };

    struct Punctuation {
        const char *        p;      ///< punctuation character(s)
        PuncType::Enum      n;      ///< punctuation id
    };

    Lexer(int flags = 0);
    Lexer(const char *text, int size, const char *filename, int flags = 0);
    ~Lexer();

    void                    Init(int flags);
    void                    Free();

    void                    SetPunctuations(const Punctuation *punctuations);

    bool                    Load(const char *text, int size, const char *name, int startLine = 0);

    bool                    EndOfFile() const { return ((ptr >= endPtr) ? true : false); }
    int                     GetCurrentOffset() const { return (int)(ptr - buffer); }

    const char *            GetFilename() const { return filename; }

    void                    Warning(const char *fmt, ...);
    void                    Error(const char *fmt, ...);
    const char *            GetLastErrorMessage() const { return errorMessage; }
    
    bool                    ReadToken(Str *token, bool allowLineBreaks = true);
    void                    UnreadToken(const Str *token);
    int                     GetTokenType() const { return tokenType; }
    int                     GetPunctuationType() const { return punctuationType; }
    int                     LinesCrossed() const { return linesCrossed; }

    int                     CheckTokenString(const char *string, bool allowLineBreaks = true);

    float                   ParseNumber();
    int                     ParseInt();
    float                   ParseFloat();
    bool                    ParseVec(int num, float *v);
    int                     Parse1DMatrix(int x, float *m);
    int                     Parse2DMatrix(int y, int x, float *m);
    int                     Parse3DMatrix(int z, int y, int x, float *m);
    const char *            ParseBracedSectionExact(Str &out, int tabs = -1);
    const char *            ParseBracedSection(Str &out);
    
    bool                    ExpectTokenType(int tokenType, Str *token);
    bool                    ExpectTokenString(const char *string, bool caseSensitive = true);
    bool                    ExpectPunctuation(int type);

    void                    SkipWhitespace();
    void                    SkipRestOfLine();
    void                    SkipBracedSection(bool parseFirstBrace = true);
    bool                    SkipUntilString(const char *string);

    int                     WhiteSpaceBeforeToken() const;

private:
    bool                    ReadString(Str *token, int quote);
    bool                    ReadNumber(Str *token);
    bool                    ReadIdentifier(Str *token);
    bool                    ReadPunctuation(Str *token);

    const char *            GetPunctuationString(int type) const;

    char                    filename[MaxRelativePath];
    char                    errorMessage[256];
    const char *            buffer;
    int                     size;
    const char *            ptr;
    const char *            lastPtr;
    const char *            endPtr;
    int                     line;
    int                     linesCrossed;
    const char *            whiteSpaceBegin_p;
    const char *            whiteSpaceEnd_p;
    bool                    loaded;
    bool                    alloced;
    int                     tokenType;
    int                     punctuationType;
    Str                     token;
    bool                    tokenAvailable;
    const Punctuation *     punctuations;
    int                     flags;
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
