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
#include "Core/Lexer.h"
#include "Core/CVars.h"
#include "Core/CmdArgs.h"

BE_NAMESPACE_BEGIN

const char *CmdArgs::Args(int start, int end) const {
    static char args[MaxCommandString];

    if (end < 0 || end >= numArgs) {
        end = numArgs - 1;
    }

    args[0] = '\0';

    for (int i = start; i <= end; i++) {
        if (i > start) {
            Str::Append(args, COUNT_OF(args), " ");
        }

        // escape string
        if (Str::FindChar(argPtrs[i], '\\')) {
            for (const char *p = argPtrs[i]; *p != '\0'; p++) {
                if (*p == '\\') {
                    Str::Append(args, COUNT_OF(args), "\\\\");
                } else {
                    int l = Str::Length(args);
                    args[l] = *p;
                    args[l + 1] = '\0';
                }
            }
        } else {
            Str::Append(args, COUNT_OF(args), argPtrs[i]);
        }
    }

    return args;
}

void CmdArgs::AppendArg(const char *arg) {
    if (!numArgs) {
        numArgs = 1;
        argPtrs[0] = tokenized;
        Str::Copynz(tokenized, arg, COUNT_OF(tokenized));
    } else if (numArgs < MaxCommandArgs) {
        argPtrs[numArgs] = argPtrs[numArgs - 1] + Str::Length(argPtrs[numArgs - 1]) + 1;
        Str::Copynz(argPtrs[numArgs], arg, COUNT_OF(tokenized) - (int)(argPtrs[numArgs] - tokenized));
        numArgs++;
    }
}

void CmdArgs::TokenizeString(const char *text, bool keepAsStrings) {
    numArgs = 0;

    if (!text) {
        return;
    }

    Lexer lexer;
    Str token;
    lexer.Init(LexerFlag::LEXFL_NOERRORS | LexerFlag::LEXFL_NOWARNINGS);
    lexer.Load(text, Str::Length(text), "CmdArgs::TokenizeString");

    int totalLen = 0;

    while (1) {
        if (numArgs == MaxCommandArgs) {
            return; // this is usually something malicious
        }

        if (!lexer.ReadToken(&token)) {
            break;
        }

        // check for negative numbers
        if (!keepAsStrings && (token == "-")) {
            if (lexer.GetTokenType() & TokenType::TT_NUMBER) {
                token = "-" + token;
            }
        }

        // check for cvar expansion
        if (token == "$") {
            if (!lexer.ReadToken(&token)) {
                return;
            }
            
            if (cvarSystem.IsInitialized()) {
                token = cvarSystem.GetCVarString(token.c_str());
            } else {
                token = "<unknown>";
            }
        }

        int len = token.Length();

        if (totalLen + len + 1 > COUNT_OF(tokenized)) {
            return; // this is usually something malicious
        }

        // regular token
        argPtrs[numArgs] = tokenized + totalLen;
        numArgs++;

        Str::Copynz(tokenized + totalLen, token.c_str(), COUNT_OF(tokenized) - totalLen);

        totalLen += len + 1;
    }
}

BE_NAMESPACE_END
