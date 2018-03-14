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
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Core/Heap.h"
#include "Core/Lexer.h"
#include "Core/WLexer.h"

BE_NAMESPACE_BEGIN

// NOTE: longer punctuations first
static const WLexer::Punctuation default_punctuations[] = {
    // binary operators
    { L">>=", PuncType::P_RSHIFT_ASSIGN },
    { L"<<=", PuncType::P_LSHIFT_ASSIGN },
    //
    { L"...", PuncType::P_PARMS },
    // define merge operator
    { L"##", PuncType::P_PRECOMPMERGE },            // pre-compiler
    // logic operators
    { L"&&", PuncType::P_LOGIC_AND },               // pre-compiler
    { L"||", PuncType::P_LOGIC_OR },                // pre-compiler
    { L">=", PuncType::P_LOGIC_GEQ },               // pre-compiler
    { L"<=", PuncType::P_LOGIC_LEQ },               // pre-compiler
    { L"==", PuncType::P_LOGIC_EQ },                // pre-compiler
    { L"!=", PuncType::P_LOGIC_UNEQ },              // pre-compiler
    // arithmatic operators
    { L"*=", PuncType::P_MUL_ASSIGN },
    { L"/=", PuncType::P_DIV_ASSIGN },
    { L"%=", PuncType::P_MOD_ASSIGN },
    { L"+=", PuncType::P_ADD_ASSIGN },
    { L"-=", PuncType::P_SUB_ASSIGN },
    { L"++", PuncType::P_INC },
    { L"--", PuncType::P_DEC },
    // binary operators
    { L"&=", PuncType::P_BIN_AND_ASSIGN },
    { L"|=", PuncType::P_BIN_OR_ASSIGN },
    { L"^=", PuncType::P_BIN_XOR_ASSIGN },
    { L">>", PuncType::P_RSHIFT },                  // pre-compiler
    { L"<<", PuncType::P_LSHIFT },                  // pre-compiler
    // reference operators
    { L"->", PuncType::P_POINTERREF },
    // C++
    { L"::", PuncType::P_CPP1 },
    { L".*", PuncType::P_CPP2 },
    // arithmatic operators
    { L"*", PuncType::P_MUL },                      // pre-compiler
    { L"/", PuncType::P_DIV },                      // pre-compiler
    { L"%", PuncType::P_MOD },                      // pre-compiler
    { L"+", PuncType::P_ADD },                      // pre-compiler
    { L"-", PuncType::P_SUB },                      // pre-compiler
    { L"=", PuncType::P_ASSIGN },
    // binary operators
    { L"&", PuncType::P_BIN_AND },                  // pre-compiler
    { L"|", PuncType::P_BIN_OR },                   // pre-compiler
    { L"^", PuncType::P_BIN_XOR },                  // pre-compiler
    { L"~", PuncType::P_BIN_NOT },                  // pre-compiler
    // logic operators
    { L"!", PuncType::P_LOGIC_NOT },                // pre-compiler
    { L">", PuncType::P_LOGIC_GREATER },            // pre-compiler
    { L"<", PuncType::P_LOGIC_LESS },               // pre-compiler
    // reference operator
    { L".", PuncType::P_REF },
    // seperators
    { L",", PuncType::P_COMMA },                    // pre-compiler
    { L";", PuncType::P_SEMICOLON },
    // label indication
    { L":", PuncType::P_COLON },                    // pre-compiler
    // if statement
    { L"?", PuncType::P_QUESTIONMARK },             // pre-compiler
    // embracements
    { L"(", PuncType::P_PARENTHESESOPEN },          // pre-compiler
    { L")", PuncType::P_PARENTHESESCLOSE },         // pre-compiler
    { L"{", PuncType::P_BRACEOPEN },                // pre-compiler
    { L"}", PuncType::P_BRACECLOSE },               // pre-compiler
    { L"[", PuncType::P_SQBRACKETOPEN },
    { L"]", PuncType::P_SQBRACKETCLOSE },
    //
    { L"\\", P_BACKSLASH }, 
    // precompiler operator
    { L"#", PuncType::P_PRECOMP },                  // pre-compiler
    { L"$", PuncType::P_DOLLAR },
    { nullptr, PuncType::P_NONE }
};

void WLexer::Init(int flags) {
    this->flags = flags;
    this->loaded = false;
    this->alloced = false;
    this->filename[0] = '\0';
    this->size = 0;
    this->line = 0;
    this->tokenAvailable = false;
    this->tokenType = 0;
    this->token.Clear();
    this->punctuations = default_punctuations;
}

void WLexer::Free() {
    if (this->alloced) {
        Mem_Free((void *)this->buffer);
        this->buffer = nullptr;
        this->alloced = false;
    }

    this->tokenAvailable = false;
    this->tokenType = 0;
    this->token.Clear();
    this->loaded = false;
}

bool WLexer::Load(const wchar_t *text, int size, const char *name, int startLine) {
    strcpy(this->filename, name);
    this->buffer = text;
    this->size = size;
    this->ptr = this->buffer;
    this->lastPtr = this->buffer;
    this->endPtr = &this->buffer[size];
    this->line = 1;
    this->loaded = true;
    this->alloced = false;
    this->tokenAvailable = false;

    return true;
}

void WLexer::Warning(const wchar_t *fmt, ...) {
    static wchar_t text[4096];
    va_list argptr;

    if (flags & LexerFlag::LEXFL_NOWARNINGS) {
        return;
    }

    va_start(argptr, fmt);
    ::vswprintf(text, COUNT_OF(text), fmt, argptr);
    va_end(argptr);

    BE_WARNLOG(L"%hs(%i) : %ls\n", filename, line, text);
}

void WLexer::Error(const wchar_t *fmt, ...) {
    static wchar_t text[4096];
    va_list argptr;
    
    if (flags & LexerFlag::LEXFL_NOERRORS) {
        return;
    }

    va_start(argptr, fmt);
    ::vswprintf(text, COUNT_OF(text), fmt, argptr);
    va_end(argptr);

    BE_ERRLOG(L"%hs(%i) : %ls\n", filename, line, text);
}

void WLexer::SetPunctuations(const Punctuation *punctuations) {
    this->punctuations = punctuations;
}

const wchar_t *WLexer::GetPunctuationString(int type) const {
    for (const Punctuation *p = punctuations; p->p; p++) {
        if (p->n == type) {
            return p->p;
        }
    }

    return L"invalid punctuation";
}

bool WLexer::ReadString(WStr *token, wchar_t quote) {
    int c;

    if (quote == L'\"') {
        tokenType = TokenType::TT_STRING;
    } else {
        tokenType = TokenType::TT_LITERAL;
    }

    // leading quote
    ptr++;

    while (1) {
        c = *ptr++;
        
        // if trailing quote
        if (c == quote) {
            return true;
        } else {
            if (c == L'\0') {
                Error(L"missing trailing quote");
                return false;
            }

            if (c == L'\n') {
                Error(L"new line inside string");
                return false;
            }

            token->Append(c);
        }
    }

    return false;
}

bool WLexer::ReadNumber(WStr *token) {
    int i;
    bool dot;

    tokenType = TokenType::TT_NUMBER;

    int c = ptr[0];
    int c2 = ptr[1];

    if (c == L'0' && c2 != L'.') {
        if (c2 == L'x' || c2 == L'X') { 
            // check for hexadecimal number
            token->Append(*ptr++);
            token->Append(*ptr++);
            while ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) {
                token->Append(c);
                c = *(++ptr);
            }

            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_HEXADECIMAL);
        } else if (c2 >= L'0' && c2 <= L'7') { 
            // check for octal number
            token->Append(*ptr++);
            c = *ptr;
            while (c >= L'0' && c <= L'7') {
                token->Append(c);
                c = *(++ptr);
            }
            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_OCTAL);
        } else { 
            // it's decimal zero case
            token->Append(*ptr++);
            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_DECIMAL);
        }
    } else {
        // decimal integer or float point number
        dot = false;

        while (1) {
            if (c >= L'0' && c <= L'9') {
                ;
            } else if (dot == false && c == L'.') {
                dot = true;
            } else {
                break;
            }
            
            token->Append(c);
            c = *(++ptr);
        }

        // if a floating point number
        if (dot) {
            tokenType |= TokenType::TT_FLOAT;
            
            if (c == L'e') {
                // check for floating point exponent
                token->Append(c);
                c = *(++ptr);
                if (c == L'-') {
                    token->Append(c);
                    c = *(++ptr);
                } else if (c == L'+') {
                    token->Append(c);
                    c = *(++ptr);
                }

                while (c >= L'0' && c <= L'9') {
                    token->Append(c);
                    c = *(++ptr);
                }
            } else if (c == L'#') {
                // check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
                tokenType |= TokenType::TT_NAN;

                c2 = 0;

                if (WStr::Cmp(ptr+1, L"INF")) {
                    tokenType |= TokenType::TT_INFINITE;
                    c2 = 3;
                } else if (WStr::Cmp(ptr+1, L"IND")) {
                    tokenType |= TokenType::TT_INDEFINITE;
                    c2 = 3;
                } else if (WStr::Cmp(ptr+1, L"NAN")) {
                    c2 = 3;
                } else if (WStr::Cmp(ptr+1, L"QNAN") || WStr::Cmp(ptr+1, L"SNAN")) {
                    c2 = 4;
                }

                if (c2 > 0) {
                    for (i = 0; i < c2; i++) {
                        token->Append(c);
                        c = *(++ptr);
                    }

                    while (c >= L'0' && c <= L'9') {
                        token->Append(c);
                        c = *(++ptr);
                    }

                    if (!(flags & LexerFlag::LEXFL_ALLOW_FLOAT_NAN)) {
                        Error(L"parsed %ls", token->c_str());
                        return false;
                    }
                }
            }
        } else {
            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_DECIMAL);
        }
    }

    if (tokenType & TokenType::TT_FLOAT) {
        if (c > L' ') {
            if (c == L'f' || c == L'F') {
                // single-precision: float
                tokenType |= TokenType::TT_SINGLE_PRECISION;
                ptr++;
            } else if (c == L'l' || c == L'L') {
                // extended-precision: long double
                tokenType |= TokenType::TT_EXTENDED_PRECISION;
                ptr++;
            } else {
                // default is double-precision: double
                tokenType |= TokenType::TT_DOUBLE_PRECISION;
            }
        } else {
            tokenType |= TokenType::TT_DOUBLE_PRECISION;
        }
    } else if (tokenType & TokenType::TT_INTEGER) {
        if (c > L' ') {
            // default: signed long
            for (i = 0; i < 2; i++) {
                if (c == L'l' || c == L'L') {
                    // long integer
                    tokenType |= TokenType::TT_LONG;
                } else if (c == L'u' || c == L'U') {
                    // unsigned integer
                    tokenType |= TokenType::TT_UNSIGNED;
                } else {
                    break;
                }
                c = *(++ptr);
            }
        }
    }

    return true;
}

bool WLexer::ReadIdentifier(WStr *token) {
    tokenType = TokenType::TT_IDENTIFIER;

    int c = *ptr;
    do {
        token->Append(c);
        c = (*++ptr);
    } while ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'@' || c > 0xFF);

    return true;
}

bool WLexer::ReadPunctuation(WStr *token) {
    int i;

    for (const Punctuation *punc = punctuations; punc->p; punc++) {
        for (i = 0; punc->p[i] && ptr[i]; i++) {
            if (ptr[i] != punc->p[i]) {
                break;
            }
        }

        if (!punc->p[i]) {
            ptr += wcslen(punc->p);
            token->Append(punc->p);
            tokenType = TokenType::TT_PUNCTUATION;
            punctuationType = punc->n;
            return true;
        }
    }
    
    return false;
}

bool WLexer::ReadToken(WStr *token, bool allowLineBreaks) {
    int	c;
    int oldLine;

    if (!loaded) {
        BE_ERRLOG(L"WLexer::ReadToken: no file loaded\n");
        return false;
    }

    // if there is a token available (from UnreadToken)
    if (tokenAvailable) {
        *token = this->token;
        tokenAvailable = false;
        return true;
    }

    if (!ptr || EndOfFile()) {
        return false;
    }
    
    lastPtr = ptr;
    token->Clear();

    whiteSpaceBegin_p = ptr;
    linesCrossed = 0;
    
    while (1) {
        // skip whitespace
        oldLine = line;
        SkipWhitespace();

        if (*ptr == L'\0' || (!allowLineBreaks && line > oldLine)) {
            return false;
        }

        linesCrossed += line - oldLine;
        
        c = *ptr;

        if (flags & LexerFlag::LEXFL_IGNORE_COMMENTS) {
            break;
        }
        
        if (c == L'/' && ptr[1] == L'/') {
            // skip double slash comments
            ptr += 2;
            while (*ptr && *ptr != L'\n') {
                ptr++;
            }
        } else if (c == L'/' && ptr[1] == L'*') {
            // skip /* */ comments
            ptr += 2;
            while (*ptr && (*ptr != L'*' || ptr[1] != L'/')) {
                ptr++;
            }
            if (*ptr) {
                ptr += 2;
            }
        } else {
            break;
        }
    }

    whiteSpaceEnd_p = ptr;
    
    if (c == L'\"' || c == L'\'') {
        // handle quoted strings
        if (!ReadString(token, c)) {
            return false;
        }
    } else if ((c >= L'0' && c <= L'9') || (c == L'.' && ptr[1] >= L'0' && ptr[1] <= L'9')) {
        // if there is a number
        if (!ReadNumber(token)) {
            return false;
        }
    } else if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_' || c == L'@' || c > 0xFF) {
        // if there is a identifier
        if (!ReadIdentifier(token)) {
            return false;
        }
    } else {
        // check for punctuation
        if (!ReadPunctuation(token)) {
            Error(L"unknown character '%c'", *ptr);
            return false;
        }
    }
    
    return true;
}

void WLexer::UnreadToken(const WStr *token) {
    if (tokenAvailable) {
        BE_FATALERROR(L"WLexer::UnreadToken, unread token twice\n");
    }

    this->token = *token;
    tokenAvailable = true;
}

int WLexer::CheckTokenString(const wchar_t *string, bool allowLineBreaks) {
    WStr tok;

    if (!ReadToken(&tok, allowLineBreaks)) {
        return 0;
    }

    // if the given string is available
    if (tok == string) {
        return 1;
    }

    // unread token
    this->token = tok;
    tokenAvailable = true;
    return 0;
}

float WLexer::ParseNumber() {
    WStr token;

    if (!ReadToken(&token)) {
        Error(L"couldn't read expected number");
        return 0;
    }
    
    if (tokenType == TokenType::TT_PUNCTUATION && token == L"-") {
        ExpectTokenType(TokenType::TT_NUMBER, &token);
        return -(float)wcstof(token.c_str(), nullptr);
    } else if (!(tokenType & TokenType::TT_NUMBER)) {
        Error(L"expected number, found '%ls'", token.c_str());
    }
    return (float)wcstof(token.c_str(), nullptr);
}

int WLexer::ParseInt() {
    WStr token;

    if (!ReadToken(&token)) {
        Error(L"couldn't read expected integer");
        return 0;
    }

    if (tokenType == TokenType::TT_PUNCTUATION && token == L"-") {
        ExpectTokenType(TokenType::TT_NUMBER | TokenType::TT_INTEGER, &token);
        return -((signed int)wcstol(token.c_str(), nullptr, 10));
    } else if ((tokenType & (TokenType::TT_NUMBER | TokenType::TT_INTEGER)) != (TokenType::TT_NUMBER | TokenType::TT_INTEGER)) {
        Error(L"expected integer value, found '%ls'", token.c_str());
    }
    return (int)wcstol(token.c_str(), nullptr, 10);
}

float WLexer::ParseFloat() {
    WStr token;

    if (!ReadToken(&token)) {
        Error(L"couldn't read expected floating point number");
        return 0;
    }

    if (tokenType == TokenType::TT_PUNCTUATION && token == L"-") {
        ExpectTokenType(TokenType::TT_NUMBER, &token);
        return -(float)wcstof(token.c_str(), nullptr);
    } else if ((tokenType & (TokenType::TT_NUMBER | TokenType::TT_FLOAT)) != (TokenType::TT_NUMBER | TokenType::TT_FLOAT)) {
        Error(L"expected float value, found '%ls'", token.c_str());
    }
    return (float)wcstof(token.c_str(), nullptr);
}

bool WLexer::ParseVec(int num, float *v) {
    if (!ExpectTokenString(L"(")) {
        return false;
    }
    
    for (int i = 0; i < num; i++) {
        v[i] = ParseNumber();
    }

    if (!ExpectTokenString(L")")) {
        return false;
    }
    
    return true;
}

int	WLexer::Parse1DMatrix(int x, float *m) {
    if (!ExpectTokenString(L"(")) {
        return false;
    }
    
    for (int i = 0; i < x; i++) {
        m[i] = ParseNumber();
    }
    
    if (!ExpectTokenString(L")")) {
        return false;
    }
    
    return true;
}

int WLexer::Parse2DMatrix(int y, int x, float *m) {
    if (!ExpectTokenString(L"(")) {
        return false;
    }

    for (int i = 0; i < y; i++) {
        if (!Parse1DMatrix(x, m + i * x)) {
            return false;
        }
    }

    if (!ExpectTokenString(L")")) {
        return false;
    }

    return true;
}

int WLexer::Parse3DMatrix(int z, int y, int x, float *m) {
    if (!ExpectTokenString(L"(")) {
        return false;
    }

    for (int i = 0; i < z; i++) {
        if (!Parse2DMatrix(y, x, m + i * x * y)) {
            return false;
        }
    }

    if (!ExpectTokenString(L")")) {
        return false;
    }

    return true;
}

// The next token should be an open brace.
// Parses until a matching close brace is found.
// Maintains exact characters between braces.
//
// FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
const wchar_t *WLexer::ParseBracedSectionExact(WStr &out, int tabs) {
    out.Clear();

    if (!WLexer::ExpectTokenString(L"{")) {
        return out.c_str();
    }
    
    out = L"{";
    int depth = 1;
    bool skipWhite = false;
    bool doTabs = tabs >= 0;

    while (depth && *WLexer::ptr) {
        wchar_t c = *(WLexer::ptr++);

        switch (c) {
        case L'\t':
        case ' ':
            if (skipWhite)
                continue;
            break;
        case L'\n':
            if (doTabs) 
            {
                skipWhite = true;
                out += c;
                continue;
            }
            break;
        case L'{':
            depth++;
            tabs++;
            break;
        case L'}':
            depth--;
            tabs--;
            break;
        }

        if (skipWhite) {
            int i = tabs;
            if (c == L'{')
                i--;
            
            skipWhite = false;
            for (; i > 0; i--) {
                out += L'\t';	
            }
        }
        out += c;
    }
    return out.c_str();
}

// The next token should be an open brace.
// Parses until a matching close brace is found.
// Internal brace depths are properly skipped.
const wchar_t *WLexer::ParseBracedSection(WStr &out) {
    WStr token;

    out.Clear();
    if (!WLexer::ExpectTokenString(L"{")) {
        return out.c_str();
    }

    out = L"{";
    int depth = 1;
    do {
        if (!WLexer::ReadToken(&token)) {
            Error(L"missing closing brace");
            return out.c_str();
        }

        // if the token is on a new line
        for (int i = 0; i < linesCrossed; i++) {
            out += L"\r\n";
        }
        
        if (WLexer::GetTokenType() == TokenType::TT_PUNCTUATION) {
            if (token[0] == L'{') {
                depth++;
            }

            else if (token[0] == L'}') {
                depth--;
            }
        }

        if (WLexer::GetTokenType() == TokenType::TT_STRING) {
            out += L"\"" + token + L"\"";
        } else {
            out += token;
        }
        out += L" ";
    } while (depth);

    return out.c_str();
}

bool WLexer::ExpectTokenType(int type, WStr *token) {
    Str str;

    if (!ReadToken(token)) {
        Error(L"couldn't read expected token");
        return false;
    }

    if ((tokenType & type) != type) {
        if ((tokenType & type) & TokenType::TT_NUMBER) {
            str.Clear();
            if (type & TokenType::TT_INTEGER) {
                if (type & TokenType::TT_DECIMAL)
                    str = "decimal ";
                if (type & TokenType::TT_HEXADECIMAL)
                    str = "hex ";
                if (type & TokenType::TT_OCTAL)
                    str = "octal ";
                if (type & TokenType::TT_UNSIGNED)
                    str += "unsigned ";
                if (type & TokenType::TT_LONG)
                    str += "long ";

                str += "integer";
            } else if (type & TokenType::TT_FLOAT) {
                str = "float";
            } else {
                str = "unknown number type";
            }

            Error(L"expected %hs but found '%ls'", str.c_str(), token->c_str());
            return false;
        } else {
            switch (type) {
            case TokenType::TT_STRING:
                str = "string";
                break;
            case TokenType::TT_LITERAL:
                str = "literal"; 
                break;
            case TokenType::TT_NUMBER:
                str = "number"; 
                break;
            case TokenType::TT_IDENTIFIER:
                str = "name"; 
                break;
            case TokenType::TT_PUNCTUATION:
                str = "punctuation"; 
                break;
            default: 
                str = "unknown type"; 
                break;
            }
            Error(L"expected a %hs but found '%ls'", str.c_str(), token->c_str());
            return false;
        }
    }
    
    return true;
}

bool WLexer::ExpectTokenString(const wchar_t *string, bool caseSensitive) {
    WStr token;

    if (!ReadToken(&token)) {
        Error(L"couldn't read expected string '%ls'", string);
        return false;
    }
        
    if (caseSensitive ? token.Cmp(string) : token.Icmp(string)) {
        Error(L"expected '%ls' but found '%ls'", string, token.c_str());
        return false;
    }

    return true;
}

bool WLexer::ExpectPunctuation(int type) {
    WStr token;

    if (!ExpectTokenType(TokenType::TT_PUNCTUATION, &token)) {
        return false;
    }

    if (punctuationType != type) {
        Error(L"expected '%ls' but found '%ls'", GetPunctuationString(type), GetPunctuationString(punctuationType));
        return false;
    }

    return true;
}

void WLexer::SkipWhitespace() {
    wchar_t c;

    while ((c = *(wchar_t *)ptr) <= L' ') {
        if (c == L'\0') {
            break;
        }

        ptr++;

        if (!c)
            return;
        
        if (c == L'\n')
            line++;
    }	
}

void WLexer::SkipRestOfLine() {
    wchar_t	c;

    const wchar_t *p = ptr;
    while ((c = *p++) != L'\0') {
        if (c == L'\n') {
            line++;
            break;
        }
    }

    ptr = p;
}

void WLexer::SkipBracedSection(bool parseFirstBrace) {
    WStr token;
    
    int depth = parseFirstBrace ? 0 : 1;
    do {
        if (!ReadToken(&token)) {
            return;
        }

        if (tokenType == TokenType::TT_PUNCTUATION) {
            if (token == L"{") {
                depth++;
            } else if (token == L"}") {
                depth--;
            }
        }
    } while (depth);
}

bool WLexer::SkipUntilString(const wchar_t *string) {
    WStr token;

    while (WLexer::ReadToken(&token)) {
        if (token == string) {
            return true;
        }
    }

    return false;
}

BE_NAMESPACE_END
