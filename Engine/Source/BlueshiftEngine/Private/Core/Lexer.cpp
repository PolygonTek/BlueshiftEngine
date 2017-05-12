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
#include "Core/Heap.h"
#include "Core/Lexer.h"

BE_NAMESPACE_BEGIN

// NOTE: longer punctuations first
static const Lexer::Punctuation default_punctuations[] = {
    // binary operators
    { ">>=", PuncType::P_RSHIFT_ASSIGN },
    { "<<=", PuncType::P_LSHIFT_ASSIGN },
    //
    { "...", PuncType::P_PARMS },
    // define merge operator
    { "##", PuncType::P_PRECOMPMERGE },           // pre-compiler
    // logic operators
    { "&&", PuncType::P_LOGIC_AND },              // pre-compiler
    { "||", PuncType::P_LOGIC_OR },               // pre-compiler
    { ">=", PuncType::P_LOGIC_GEQ },              // pre-compiler
    { "<=", PuncType::P_LOGIC_LEQ },              // pre-compiler
    { "==", PuncType::P_LOGIC_EQ },               // pre-compiler
    { "!=", PuncType::P_LOGIC_UNEQ },             // pre-compiler
    // arithmatic operators
    { "*=", PuncType::P_MUL_ASSIGN },
    { "/=", PuncType::P_DIV_ASSIGN },
    { "%=", PuncType::P_MOD_ASSIGN },
    { "+=", PuncType::P_ADD_ASSIGN },
    { "-=", PuncType::P_SUB_ASSIGN },
    { "++", PuncType::P_INC },
    { "--", PuncType::P_DEC },
    // binary operators
    { "&=", PuncType::P_BIN_AND_ASSIGN },
    { "|=", PuncType::P_BIN_OR_ASSIGN },
    { "^=", PuncType::P_BIN_XOR_ASSIGN },
    { ">>", PuncType::P_RSHIFT },                 // pre-compiler
    { "<<", PuncType::P_LSHIFT },                 // pre-compiler
    // reference operators
    { "->", PuncType::P_POINTERREF },
    // C++
    { "::", PuncType::P_CPP1 },
    { ".*", PuncType::P_CPP2 },
    // arithmatic operators
    { "*", PuncType::P_MUL },                     // pre-compiler
    { "/", PuncType::P_DIV },                     // pre-compiler
    { "%", PuncType::P_MOD },                     // pre-compiler
    { "+", PuncType::P_ADD },                     // pre-compiler
    { "-", PuncType::P_SUB },                     // pre-compiler
    { "=", PuncType::P_ASSIGN },
    // binary operators
    { "&", PuncType::P_BIN_AND },                 // pre-compiler
    { "|", PuncType::P_BIN_OR },                  // pre-compiler
    { "^", PuncType::P_BIN_XOR },                 // pre-compiler
    { "~", PuncType::P_BIN_NOT },                 // pre-compiler
    // logic operators
    { "!", PuncType::P_LOGIC_NOT },               // pre-compiler
    { ">", PuncType::P_LOGIC_GREATER },           // pre-compiler
    { "<", PuncType::P_LOGIC_LESS },              // pre-compiler
    // reference operator
    { ".", PuncType::P_REF },
    // seperators
    { ",", PuncType::P_COMMA },                   // pre-compiler
    { ";", PuncType::P_SEMICOLON },
    // label indication
    { ":", PuncType::P_COLON },                   // pre-compiler
    // if statement
    { "?", PuncType::P_QUESTIONMARK },            // pre-compiler
    // embracements
    { "(", PuncType::P_PARENTHESESOPEN },         // pre-compiler
    { ")", PuncType::P_PARENTHESESCLOSE },        // pre-compiler
    { "{", PuncType::P_BRACEOPEN },               // pre-compiler
    { "}", PuncType::P_BRACECLOSE },              // pre-compiler
    { "[", PuncType::P_SQBRACKETOPEN },
    { "]", PuncType::P_SQBRACKETCLOSE },
    //
    { "\\", PuncType::P_BACKSLASH },
    // precompiler operator
    { "#", PuncType::P_PRECOMP },                 // pre-compiler
    { "$", PuncType::P_DOLLAR },
    { nullptr, PuncType::P_NONE }
};

void Lexer::Init(int flags) {
    this->flags = flags;
    this->loaded = false;
    this->alloced = false;
    this->filename[0] = '\0';
    this->errorMessage[0] = '\0';
    this->size = 0;
    this->line = 0;
    this->tokenAvailable = false;
    this->tokenType = 0;
    this->token.Clear();
    this->punctuations = default_punctuations;
}

void Lexer::Free() {
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

bool Lexer::Load(const char *text, int size, const char *name, int startLine) {
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

void Lexer::Warning(const char *fmt, ...) {
    static char text[4096];
    va_list argptr;

    if (flags & LexerFlag::LEXFL_NOWARNINGS) {
        return;
    }

    va_start(argptr, fmt);
    ::vsprintf(text, fmt, argptr);
    va_end(argptr);

    BE_WARNLOG(L"%hs(%i) : %hs\n", filename, line, text);
}

void Lexer::Error(const char *fmt, ...) {
    static char text[4096];
    va_list argptr;
    
    if (flags & LexerFlag::LEXFL_NOERRORS) {
        return;
    }

    va_start(argptr, fmt);
    ::vsprintf(text, fmt, argptr);
    va_end(argptr);

    BE_ERRLOG(L"%hs(%i) : %hs\n", filename, line, text);
}

void Lexer::SetPunctuations(const Punctuation *punctuations) {
    this->punctuations = punctuations;
}

const char *Lexer::GetPunctuationString(int type) const {
    for (const Punctuation *p = punctuations; p->p; p++) {
        if (p->n == type) {
            return p->p;
        }
    }

    return "invalid punctuation";
}

bool Lexer::ReadString(Str *token, int quote) {
    if (quote == '\"') {
        tokenType = TokenType::TT_STRING;
    } else {
        tokenType = TokenType::TT_LITERAL;
    }

    // leading quote
    ptr++;

    while (1) {
        int c = *ptr++;
        
        // if trailing quote
        if (c == quote) {
            return true;
        } else {
            if (c == '\0') {
                Error("missing trailing quote");
                return false;
            }

            if (c == '\n') {
                Error("new line inside string");
                return false;
            }

            token->Append(c);
        }
    }

    return false;
}

bool Lexer::ReadNumber(Str *token) {
    tokenType = TokenType::TT_NUMBER;

    int c = ptr[0];
    int c2 = ptr[1];

    if (c == '0' && c2 != '.') {
        if (c2 == 'x' || c2 == 'X') {
            // check for hexadecimal number
            token->Append(*ptr++);
            token->Append(*ptr++);
            while ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                token->Append(c);
                c = *(++ptr);
            }

            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_HEXADECIMAL);
        } else if (c2 >= '0' && c2 <= '7') {
            // check for octal number
            token->Append(*ptr++);
            c = *ptr;
            while (c >= '0' && c <= '7') {
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
        bool dot = false;

        while (1) {
            if (c >= '0' && c <= '9') {
                ;
            } else if (dot == false && c == '.') {
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
            
            if (c == 'e') {
                // check for floating point exponent
                token->Append(c);
                c = *(++ptr);
                if (c == '-') {
                    token->Append(c);
                    c = *(++ptr);
                } else if (c == '+') {
                    token->Append(c);
                    c = *(++ptr);
                }

                while (c >= '0' && c <= '9') {
                    token->Append(c);
                    c = *(++ptr);
                }
            } else if (c == '#') {
                // check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
                tokenType |= TokenType::TT_NAN;

                c2 = 0;

                if (Str::Cmp(ptr+1, "INF")) {
                    tokenType |= TokenType::TT_INFINITE;
                    c2 = 3;
                } else if (Str::Cmp(ptr+1, "IND")) {
                    tokenType |= TokenType::TT_INDEFINITE;
                    c2 = 3;
                } else if (Str::Cmp(ptr+1, "NAN")) {
                    c2 = 3;
                } else if (Str::Cmp(ptr+1, "QNAN") || Str::Cmp(ptr+1, "SNAN")) {
                    c2 = 4;
                }

                if (c2 > 0) {
                    for (int i = 0; i < c2; i++) {
                        token->Append(c);
                        c = *(++ptr);
                    }

                    while (c >= '0' && c <= '9') {
                        token->Append(c);
                        c = *(++ptr);
                    }

                    if (!(flags & LexerFlag::LEXFL_ALLOW_FLOAT_NAN)) {
                        Error("parsed %s", token->c_str());
                        return false;
                    }
                }
            }
        } else {
            tokenType |= (TokenType::TT_INTEGER | TokenType::TT_DECIMAL);
        }
    }

    if (tokenType & TokenType::TT_FLOAT) {
        if (c > ' ') {
            if (c == 'f' || c == 'F') {
                // single-precision: float
                tokenType |= TokenType::TT_SINGLE_PRECISION;
                ptr++;
            } else if (c == 'l' || c == 'L') {
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
        if (c > ' ') {
            // default: signed long
            for (int i = 0; i < 2; i++) {
                if (c == 'l' || c == 'L') {
                    // long integer
                    tokenType |= TokenType::TT_LONG;
                } else if (c == 'u' || c == 'U') {
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

bool Lexer::ReadIdentifier(Str *token) {
    tokenType = TokenType::TT_IDENTIFIER;

    int c = *ptr;
    do {
        token->Append(c);
        c = (*++ptr);
    } while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '@');

    return true;
}

bool Lexer::ReadPunctuation(Str *token) {
    int i;

    for (const Punctuation *punc = punctuations; punc->p; punc++) {
        for (i = 0; punc->p[i] && ptr[i]; i++) {
            if (ptr[i] != punc->p[i]) {
                break;
            }
        }

        if (!punc->p[i]) {
            ptr += strlen(punc->p);
            token->Append(punc->p);
            tokenType = TokenType::TT_PUNCTUATION;
            punctuationType = punc->n;
            return true;
        }
    }
    
    return false;
}

bool Lexer::ReadToken(Str *token, bool allowLineBreaks) {
    int	c;
    int oldLine;

    if (!loaded) {
        BE_ERRLOG(L"Lexer::ReadToken: no file loaded\n");
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

        if (*ptr == '\0' || (!allowLineBreaks && line > oldLine)) {
            return false;
        }

        linesCrossed += line - oldLine;
        
        c = *ptr;

        if (flags & LexerFlag::LEXFL_IGNORE_COMMENTS) {
            break;
        }

        if (c == '/' && ptr[1] == '/') {
            // skip double slash comments
            ptr += 2;
            while (*ptr && *ptr != '\n') {
                ptr++;
            }
        } else if (c == '/' && ptr[1] == '*') {
            // skip /* */ comments
            ptr += 2;
            while (*ptr && (*ptr != '*' || ptr[1] != '/')) {
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
    
    if (c == '\"' || c == '\'') {
        // handle quoted strings
        if (!ReadString(token, c)) {
            return false;
        }
    } else if ((c >= '0' && c <= '9') || (c == '.' && ptr[1] >= '0' && ptr[1] <= '9')) {
        // if there is a number
        if (!ReadNumber(token)) {
            return false;
        }
    } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '@') {
        // if there is a identifier
        if (!ReadIdentifier(token)) {
            return false;
        }
    } else {
        // check for punctuation
        if (!ReadPunctuation(token)) {
            Error("unknown character '%c'", *ptr);
            return false;
        }
    }
    
    return true;
}

void Lexer::UnreadToken(const Str *token) {
    if (tokenAvailable) {
        BE_FATALERROR(L"Lexer::UnreadToken, unread token twice\n");
    }

    this->token = *token;
    tokenAvailable = true;
}

int Lexer::CheckTokenString(const char *string, bool allowLineBreaks) {
    Str tok;

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

float Lexer::ParseNumber() {
    Str token;

    if (!ReadToken(&token)) {
        Error("couldn't read expected number");
        return 0;
    }

    if (tokenType == TokenType::TT_PUNCTUATION && token == "-") {
        ExpectTokenType(TokenType::TT_NUMBER, &token);
        return -(float)atof(token.c_str());
    } else if (!(tokenType & TokenType::TT_NUMBER)) {
        Error("expected number, found '%s'", token.c_str());
    }
    return (float)atof(token.c_str());
}

int Lexer::ParseInt() {
    Str token;

    if (!ReadToken(&token)) {
        Error("couldn't read expected integer");
        return 0;
    }

    if (tokenType == TokenType::TT_PUNCTUATION && token == "-") {
        ExpectTokenType(TokenType::TT_NUMBER | TokenType::TT_INTEGER, &token);
        return -((signed int)atoi(token.c_str()));
    } else if ((tokenType & (TokenType::TT_NUMBER | TokenType::TT_INTEGER)) != (TokenType::TT_NUMBER | TokenType::TT_INTEGER)) {
        Error("expected integer value, found '%s'", token.c_str());
    }
    return atoi(token.c_str());
}

float Lexer::ParseFloat() {
    Str token;

    if (!ReadToken(&token)) {
        Error("couldn't read expected floating point number");
        return 0;
    }

    if (tokenType == TokenType::TT_PUNCTUATION && token == "-") {
        ExpectTokenType(TokenType::TT_NUMBER, &token);
        return -(float)atof(token.c_str());
    } else if ((tokenType & (TokenType::TT_NUMBER | TokenType::TT_FLOAT)) != (TokenType::TT_NUMBER | TokenType::TT_FLOAT)) {
        Error("expected float value, found '%s'", token.c_str());
    }
    return (float)atof(token.c_str());
}

bool Lexer::ParseVec(int num, float *v) {
    if (!ExpectTokenString("(")) {
        return false;
    }
    
    for (int i = 0; i < num; i++) {
        v[i] = ParseNumber();
    }

    if (!ExpectTokenString(")")) {
        return false;
    }
    
    return true;
}

int Lexer::Parse1DMatrix(int x, float *m) {
    if (!ExpectTokenString("(")) {
        return false;
    }
    
    for (int i = 0; i < x; i++) {
        m[i] = ParseNumber();
    }
    
    if (!ExpectTokenString(")")) {
        return false;
    }
    
    return true;
}

int Lexer::Parse2DMatrix(int y, int x, float *m) {
    if (!ExpectTokenString("(")) {
        return false;
    }

    for (int i = 0; i < y; i++) {
        if (!Parse1DMatrix(x, m + i * x)) {
            return false;
        }
    }

    if (!ExpectTokenString(")")) {
        return false;
    }

    return true;
}

int Lexer::Parse3DMatrix(int z, int y, int x, float *m) {
    if (!ExpectTokenString("(")) {
        return false;
    }

    for (int i = 0; i < z; i++) {
        if (!Parse2DMatrix(y, x, m + i * x * y)) {
            return false;
        }
    }

    if (!ExpectTokenString(")")) {
        return false;
    }

    return true;
}

// The next token should be an open brace.
// Parses until a matching close brace is found.
// Maintains exact characters between braces.
//
// FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
const char *Lexer::ParseBracedSectionExact(Str &out, int tabs) {
    out.Clear();

    if (!Lexer::ExpectTokenString("{")) {
        return out.c_str();
    }
    
    int depth = 1;
    bool skipWhite = false;
    bool doTabs = tabs >= 0;

    while (depth && *Lexer::ptr) {
        char c = *(Lexer::ptr++);

        switch (c) {
        case '\t':
        case ' ':
            if (skipWhite)
                continue;
            break;
        case '\n':
            if (doTabs) {
                skipWhite = true;
                out += c;
                continue;
            }
            break;
        case '{':
            depth++;
            tabs++;
            break;
        case '}':
            depth--;
            tabs--;
            if (!depth) {
                continue;
            }
            break;
        }

        if (skipWhite) {
            int i = tabs;
            if (c == '{') {
                i--;
            }
            
            skipWhite = false;
            for (; i > 0; i--) {
                out += '\t';	
            }
        }
        out += c;
    }
    return out.c_str();
}

// The next token should be an open brace.
// Parses until a matching close brace is found.
// Internal brace depths are properly skipped.
const char *Lexer::ParseBracedSection(Str &out) {
    Str token;

    out.Clear();
    if (!Lexer::ExpectTokenString("{")) {
        return out.c_str();
    }

    int depth = 1;
    do {
        if (!Lexer::ReadToken(&token)) {
            Error("missing closing brace");
            return out.c_str();
        }

        // if the token is on a new line
        for (int i = 0; i < linesCrossed; i++) {
            out += "\r\n";
        }
        
        if (Lexer::GetTokenType() == TokenType::TT_PUNCTUATION) {
            if (token[0] == '{') {
                depth++;
            } else if (token[0] == '}') {
                depth--;
                if (!depth) {
                    continue;
                }
            }
        }

        if (Lexer::GetTokenType() == TokenType::TT_STRING) {
            out += "\"" + token + "\"";
        } else {
            out += token;
        }
        out += " ";
    } while (depth);

    return out.c_str();
}

bool Lexer::ExpectTokenType(int type, Str *token) {
    Str str;

    if (!ReadToken(token)) {
        Error("couldn't read expected token");
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

            Error("expected %s but found '%s'", str.c_str(), token->c_str());
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
            Error("expected a %s but found '%s'", str.c_str(), token->c_str());
            return false;
        }
    }
    
    return true;
}

bool Lexer::ExpectTokenString(const char *string, bool caseSensitive) {
    Str token;

    if (!ReadToken(&token)) {
        Error("couldn't read expected string '%s'", string);
        return false;
    }
        
    if (caseSensitive ? token.Cmp(string) : token.Icmp(string)) {
        Error("expected '%s' but found '%s'", string, token.c_str());
        return false;
    }

    return true;
}

bool Lexer::ExpectPunctuation(int type) {
    Str token;

    if (!ExpectTokenType(TokenType::TT_PUNCTUATION, &token)) {
        return false;
    }

    if (punctuationType != type) {
        Error("expected '%s' but found '%s'", GetPunctuationString(type), GetPunctuationString(punctuationType));
        return false;
    }

    return true;
}

void Lexer::SkipWhitespace() {
    int c;

    while ((c = *(byte *)ptr) <= ' ') {
        if (c == '\0') {
            break;
        }

        ptr++;

        if (!c) {
            return;
        }
        
        if (c == '\n') {
            line++;
        }
    }
}

void Lexer::SkipRestOfLine() {
    int c;

    const char *p = ptr;
    while ((c = *p++) != '\0') {
        if (c == '\n') {
            line++;
            break;
        }
    }

    ptr = p;
}

void Lexer::SkipBracedSection(bool parseFirstBrace) {
    Str token;
    
    int depth = parseFirstBrace ? 0 : 1;
    do {
        if (!ReadToken(&token)) {
            return;
        }

        if (tokenType == TokenType::TT_PUNCTUATION) {
            if (token == "{") {
                depth++;
            } else if (token == "}") {
                depth--;
            }
        }
    } while (depth);
}

bool Lexer::SkipUntilString(const char *string) {
    Str token;

    while (Lexer::ReadToken(&token)) {
        if (token == string) {
            return true;
        }
    }

    return false;
}

BE_NAMESPACE_END
