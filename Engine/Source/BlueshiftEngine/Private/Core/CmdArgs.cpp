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
#include "Core/Lexer.h"
#include "Core/WLexer.h"
#include "Core/CVars.h"
#include "Core/CmdArgs.h"

BE_NAMESPACE_BEGIN

const wchar_t *CmdArgs::Args(int start, int end) const {
    static wchar_t args[MaxCommandString];

    if (end < 0 || end >= numArgs) {
        end = numArgs - 1;
    }

    args[0] = L'\0';

    for (int i = start; i <= end; i++) {
        if (i > start) {
            WStr::Append(args, COUNT_OF(args), L" ");
        }

        // escape string
        if (WStr::FindChar(argPtrs[i], L'\\')) {
            for (const wchar_t *p = argPtrs[i]; *p != L'\0'; p++) {
                if (*p == L'\\') {
                    WStr::Append(args, COUNT_OF(args), L"\\\\");
                } else {
                    int l = WStr::Length(args);
                    args[l] = *p;
                    args[l + 1] = L'\0';
                }
            }
        } else {
            WStr::Append(args, COUNT_OF(args), argPtrs[i]);
        }
    }

    return args;
}

void CmdArgs::AppendArg(const wchar_t *arg) {
    if (!numArgs) {
        numArgs = 1;
        argPtrs[0] = tokenized;
        WStr::Copynz(tokenized, arg, COUNT_OF(tokenized));
    } else if (numArgs < MaxCommandArgs) {
        argPtrs[numArgs] = argPtrs[numArgs - 1] + WStr::Length(argPtrs[numArgs - 1]) + 1;
        WStr::Copynz(argPtrs[numArgs], arg, COUNT_OF(tokenized) - (int)(argPtrs[numArgs] - tokenized));
        numArgs++;
    }
}

void CmdArgs::TokenizeString(const wchar_t *text, bool keepAsStrings) {
    numArgs = 0;

    if (!text) {
        return;
    }

    WLexer lexer;
    WStr token;
    lexer.Init(LexerFlag::LEXFL_NOERRORS | LexerFlag::LEXFL_NOWARNINGS);
    lexer.Load(text, WStr::Length(text), "CmdArgs::TokenizeString");

    int totalLen = 0;

    while (1) {
        if (numArgs == MaxCommandArgs) {
            return; // this is usually something malicious
        }

        if (!lexer.ReadToken(&token)) {
            break;
        }

        // check for negative numbers
        if (!keepAsStrings && (token == L"-")) {
            if (lexer.GetTokenType() & TokenType::TT_NUMBER) {
                token = L"-" + token;
            }
        }

        // check for cvar expansion
        if (token == L"$") {
            if (!lexer.ReadToken(&token)) {
                return;
            }
            
            if (cvarSystem.IsInitialized()) {
                token = cvarSystem.GetCVarString(token.c_str());
            } else {
                token = L"<unknown>";
            }
        }

        int len = token.Length();

        if (totalLen + len + 1 > COUNT_OF(tokenized)) {
            return; // this is usually something malicious
        }

        // regular token
        argPtrs[numArgs] = tokenized + totalLen;
        numArgs++;

        WStr::Copynz(tokenized + totalLen, token.c_str(), COUNT_OF(tokenized) - totalLen);

        totalLen += len + 1;
    }
}

BE_NAMESPACE_END
