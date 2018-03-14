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
#include "Math/Math.h"
#include "Core/Lexer.h"
#include "Core/Expr.h"

BE_NAMESPACE_BEGIN

#define MAX_TEMP_REGISTERS      32
#define MAX_CONSTANT_REGISTERS  256
#define MAX_INSTRUCTIONS        65536

#define MAKE_REG(t, i)          (((t) << 16) + ((i) & 0xFFFF))
#define MAKE_REG_OUTPUT(i)      MAKE_REG(OutputRegister, (i))
#define MAKE_REG_TEMP(i)        MAKE_REG(TempRegister, (i))
#define MAKE_REG_CONSTANT(i)    MAKE_REG(ConstantRegister, (i))
#define MAKE_REG_TABLE(i)       MAKE_REG(TableRegister, (i))
#define MAKE_REG_GLOBALPARM(i)  MAKE_REG(GlobalParmRegister, (i))
#define MAKE_REG_LOCALPARM(i)   MAKE_REG(LocalParmRegister, (i))

enum ExprParseState {
    EmptyState      = BIT(0),
    LParenState     = BIT(1),
    OpState         = BIT(2),
    BuiltInOpState  = BIT(3),
    OperandState    = BIT(4)
};

struct OpCodeInfo {
    const char *    name;
    ExprChunk::OpCode opCode;
    bool            unary;
    int             precedence;     // 연산자 우선순위
};

// NOTE: 배열 순서는 헤더파일의 OpCode enum 값과 맞춰져있음
static OpCodeInfo   opCodeInfoTable[] = {
    { "",           ExprChunk::OpCode_Invalid,          false, -1 },
    { "(",          ExprChunk::OpCode_LParen,           false,  0 },        // (
    { "(",          ExprChunk::OpCode_BuiltInLParen,    false,  0 },        // (
    { "[",          ExprChunk::OpCode_Table,            false,  0 },        // [
    { "!",          ExprChunk::OpCode_Not,              true,   5 },        // !
    { "+",          ExprChunk::OpCode_Plus,             true,   5 },        // +
    { "-",          ExprChunk::OpCode_Minus,            true,   5 },        // -
    { "abs",        ExprChunk::OpCode_Abs,              true,   5 },        // abs
    { "floor",      ExprChunk::OpCode_Floor,            true,   5 },        // floor
    { "ceil",       ExprChunk::OpCode_Ceil,             true,   5 },        // ceil
    { "fract",      ExprChunk::OpCode_Fract,            true,   5 },        // fract
    { "sqrt",       ExprChunk::OpCode_Sqrt,             true,   5 },        // sqrt
    { "invsqrt",    ExprChunk::OpCode_InvSqrt,          true,   5 },        // inverse sqrt
    { "min",        ExprChunk::OpCode_Min,              false,  5 },        // min
    { "max",        ExprChunk::OpCode_Max,              false,  5 },        // max
    { "pow",        ExprChunk::OpCode_Pow,              false,  5 },        // power
    { "exp",        ExprChunk::OpCode_Exp,              true,   5 },        // exponent
    { "log",        ExprChunk::OpCode_Log,              true,   5 },        // log
    { "sin",        ExprChunk::OpCode_Sin,              true,   5 },        // sine
    { "cos",        ExprChunk::OpCode_Cos,              true,   5 },        // cosine
    { "tan",        ExprChunk::OpCode_Tan,              true,   5 },        // tangent
    { "asin",       ExprChunk::OpCode_ASin,             true,   5 },        // arc sine
    { "acos",       ExprChunk::OpCode_ACos,             true,   5 },        // arc cosine
    { "atan",       ExprChunk::OpCode_ATan,             true,   5 },        // arc tangent
    { "*",          ExprChunk::OpCode_Multiply,         false,  4 },        // *
    { "/",          ExprChunk::OpCode_Divide,           false,  4 },        // /
    { "%",          ExprChunk::OpCode_Mod,              false,  4 },        // %
    { "+",          ExprChunk::OpCode_Add,              false,  3 },        // +
    { "-",          ExprChunk::OpCode_Subtract,         false,  3 },        // -
    { ">",          ExprChunk::OpCode_GreaterThan,      false,  2 },        // >
    { ">=",         ExprChunk::OpCode_GreaterEqual,     false,  2 },        // >=
    { "<",          ExprChunk::OpCode_LessThan,         false,  2 },        // <
    { "<=",         ExprChunk::OpCode_LessEqual,        false,  2 },        // <=
    { "==",         ExprChunk::OpCode_Equal,            false,  2 },        // ==
    { "!=",         ExprChunk::OpCode_NotEqual,         false,  2 },        // !=
    { "&&",         ExprChunk::OpCode_And,              false,  1 },        // &&
    { "||",         ExprChunk::OpCode_Or,               false,  1 },        // ||
};

static int          g_opCodeStack[16];
static int          g_opCodeStackPointer; // NOTE: 선증감

static int          g_operandStack[16];
static int          g_operandStackPointer; // NOTE: 선증감

static int          g_tempRegisterUseCount;

static float        g_constantRegisters[MAX_CONSTANT_REGISTERS];
static ExprChunk::Instruction g_instructions[MAX_INSTRUCTIONS];

static float        g_tempRegisters[MAX_TEMP_REGISTERS];

static const float *g_localParms;
static float *      g_outputValues;

// punctuation type -> op code
static ExprChunk::OpCode PunctuationTypeToOpCode(int puncType, bool unary) {
    switch (puncType) {
    case P_LOGIC_NOT:       // !
        if (!unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Not;
    case P_ADD:             // +
        if (unary) return ExprChunk::OpCode_Plus;
        return ExprChunk::OpCode_Add;
    case P_SUB:             // -
        if (unary) return ExprChunk::OpCode_Minus;
        return ExprChunk::OpCode_Subtract;
    case P_MUL:             // *
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Multiply;
    case P_DIV:             // /
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Divide;
    case P_MOD:             // %
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Mod;
    case P_LOGIC_GREATER:   // >
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_GreaterThan;
    case P_LOGIC_GEQ:       // >=
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_GreaterEqual;
    case P_LOGIC_LESS:      // <
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_LessThan;
    case P_LOGIC_LEQ:       // <=
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_LessEqual;
    case P_LOGIC_EQ:        // ==
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Equal;
    case P_LOGIC_UNEQ:      // !=
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_NotEqual;
    case P_LOGIC_AND:       // &&
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_And;
    case P_LOGIC_OR:        // ||
        if (unary) return ExprChunk::OpCode_Invalid;
        return ExprChunk::OpCode_Or;
    }

    assert(0);
    return ExprChunk::OpCode_Invalid;
}

static ExprChunk::OpCode FindBuiltInOpCode(const char *name) {
    for (int i = 0; i < COUNT_OF(opCodeInfoTable); i++) {
        if (isalpha(opCodeInfoTable[i].name[0])) {
            if (!Str::Cmp(opCodeInfoTable[i].name, name)) {
                return opCodeInfoTable[i].opCode;
            }
        }
    }

    return ExprChunk::OpCode_Invalid;
}

void ExprChunk::Init(ExprData *data) {
    this->data = data;

    this->numRegisters = 0;
    
    this->numConstantRegisters = 0;
    this->constantRegisters = nullptr;

    this->numInstructions = 0;
    this->instructions = nullptr;
}

void ExprChunk::Free() {
    if (instructions) {
        Mem_Free(instructions);
        instructions = nullptr;
    }

    if (constantRegisters) {
        Mem_Free(constantRegisters);
        constantRegisters = nullptr;
    }
}

void ExprChunk::GenerateInstruction(OpCode opCode, int dest, int src0, int src1) {
    assert(numInstructions < MAX_INSTRUCTIONS);

    Instruction *inst = &g_instructions[numInstructions++];
    inst->opCode = opCode;
    inst->dest = dest;
    inst->src0 = src0;
    inst->src1 = src1;
}

void ExprChunk::EvaluateStack() {
    int     dest, src0, src1;
    int     a, b;

    bool preCalculated = false;

    assert(g_opCodeStackPointer > -1);
    OpCode opCode = (OpCode)g_opCodeStack[g_opCodeStackPointer--];

    if (opCodeInfoTable[opCode].unary) {    // unary operator
        assert(g_operandStackPointer > -1);
        src1 = 0;
        src0 = g_operandStack[g_operandStackPointer--];

        if (GET_REG_TYPE(src0) == ConstantRegister) {
            a = GET_REG_INDEX(src0);

            switch (opCode)	{
            case OpCode_Not:
                g_constantRegisters[a] = !g_constantRegisters[a];
                break;
            case OpCode_Plus:
                //g_constantRegisters[a] = +g_constantRegisters[a]; // 생략
                break;
            case OpCode_Minus:
                g_constantRegisters[a] = -g_constantRegisters[a];
                break;
            case OpCode_Abs:
                g_constantRegisters[a] = Math::Fabs(g_constantRegisters[a]);
                break;
            case OpCode_Floor:
                g_constantRegisters[a] = Math::Floor(g_constantRegisters[a]);
                break;
            case OpCode_Ceil:
                g_constantRegisters[a] = Math::Ceil(g_constantRegisters[a]);
                break;
            case OpCode_Sqrt:
                g_constantRegisters[a] = Math::Sqrt(g_constantRegisters[a]);
                break;
            case OpCode_InvSqrt:
                g_constantRegisters[a] = Math::InvSqrt(g_constantRegisters[a]);
                break;
            case OpCode_Exp:
                g_constantRegisters[a] = Math::Exp(g_constantRegisters[a]);
                break;
            case OpCode_Log:
                g_constantRegisters[a] = Math::Log(g_constantRegisters[a]);
                break;
            case OpCode_Sin:
                g_constantRegisters[a] = Math::Sin(DEG2RAD(Math::AngleNormalize360(g_constantRegisters[a])));
                break;
            case OpCode_Cos:
                g_constantRegisters[a] = Math::Cos(DEG2RAD(Math::AngleNormalize360(g_constantRegisters[a])));
                break;
            case OpCode_Tan:
                g_constantRegisters[a] = Math::Tan(DEG2RAD(Math::AngleNormalize360(g_constantRegisters[a])));
                break;
            case OpCode_ASin:
                g_constantRegisters[a] = Math::ASin(g_constantRegisters[a]);
                break;
            case OpCode_ACos:
                g_constantRegisters[a] = Math::ACos(g_constantRegisters[a]);
                break;
            case OpCode_ATan:
                g_constantRegisters[a] = Math::ATan(g_constantRegisters[a]);
                break;
            default:
                assert(0);
                break;
            }

            dest = src0;
            preCalculated = true;
        } else if ((opCode == OpCode_Plus) && (GET_REG_TYPE(src0) == GlobalParmRegister || GET_REG_TYPE(src0) == LocalParmRegister || GET_REG_TYPE(src0) == TableRegister)) {
            // + (unary) 는 연산안함
            dest = src0;
            preCalculated = true;
        }
    } else {    // binary operator
        assert(g_operandStackPointer > 0);
        src1 = g_operandStack[g_operandStackPointer--];
        src0 = g_operandStack[g_operandStackPointer--];

        // 두 오퍼랜드 모두 상수냐? 그렇다면.. 상수끼리 연산은 미리 계산한다
        if (GET_REG_TYPE(src0) == ConstantRegister && GET_REG_TYPE(src1) == ConstantRegister) {
            a = GET_REG_INDEX(src0);
            b = GET_REG_INDEX(src1);

            switch (opCode) {
            case OpCode_Min:
                g_constantRegisters[a] = Min(g_constantRegisters[a], g_constantRegisters[b]);
                break;
            case OpCode_Max:
                g_constantRegisters[a] = Max(g_constantRegisters[a], g_constantRegisters[b]);
                break;
            case OpCode_Pow:
                g_constantRegisters[a] = Math::Pow(g_constantRegisters[a], g_constantRegisters[b]);
                break;
            case OpCode_Add:
                g_constantRegisters[a] = g_constantRegisters[a] + g_constantRegisters[b];
                break;
            case OpCode_Subtract:
                g_constantRegisters[a] = g_constantRegisters[a] - g_constantRegisters[b];
                break;
            case OpCode_Multiply:
                g_constantRegisters[a] = g_constantRegisters[a] * g_constantRegisters[b];
                break;
            case OpCode_Divide:
                g_constantRegisters[a] = g_constantRegisters[a] / g_constantRegisters[b];
                break;
            case OpCode_Mod:
                g_constantRegisters[a] = (int)g_constantRegisters[a] % (int)g_constantRegisters[b];
                break;
            case OpCode_GreaterThan:
                g_constantRegisters[a] = g_constantRegisters[a] > g_constantRegisters[b];
                break;
            case OpCode_GreaterEqual:
                g_constantRegisters[a] = g_constantRegisters[a] >= g_constantRegisters[b];
                break;
            case OpCode_LessThan:
                g_constantRegisters[a] = g_constantRegisters[a] < g_constantRegisters[b];
                break;
            case OpCode_LessEqual:
                g_constantRegisters[a] = g_constantRegisters[a] <= g_constantRegisters[b];
                break;
            case OpCode_Equal:
                g_constantRegisters[a] = g_constantRegisters[a] == g_constantRegisters[b];
                break;
            case OpCode_NotEqual:
                g_constantRegisters[a] = g_constantRegisters[a] != g_constantRegisters[b];
                break;
            case OpCode_And:
                g_constantRegisters[a] = g_constantRegisters[a] && g_constantRegisters[b];
                break;
            case OpCode_Or:
                g_constantRegisters[a] = g_constantRegisters[a] || g_constantRegisters[b];
                break;
            default:
                assert(0);
                break;
            }

            dest = src0;
            preCalculated = true;
        }
    }

    if (!preCalculated) {
        if (GET_REG_TYPE(src0) == TempRegister) {
            dest = src0;
        } else {
            dest = MAKE_REG_TEMP(g_tempRegisterUseCount);
            g_tempRegisterUseCount++;
        }

        GenerateInstruction(opCode, dest, src0, src1);
    }

    g_operandStack[++g_operandStackPointer] = dest;
}

void ExprChunk::Finish() {
    if (numInstructions > 0) {
        instructions = (Instruction *)Mem_Alloc(sizeof(Instruction) * numInstructions);
        memcpy(instructions, g_instructions, sizeof(Instruction) * numInstructions);
    }

    if (numConstantRegisters > 0) {
        constantRegisters = (float *)Mem_Alloc(sizeof(float) * numConstantRegisters);
        memcpy(constantRegisters, g_constantRegisters, sizeof(float) * numConstantRegisters);	
    }
}

bool ExprChunk::ParseExpressions(const char *text, int numExpressions, int *outputRegisters) {
    Lexer lexer(text, Str::Length(text), "ExprChunk::ParseExpressions", LexerFlag::LEXFL_NOERRORS);
    return ParseExpressions(lexer, numExpressions, outputRegisters);
}

bool ExprChunk::ParseExpressions(Lexer &lexer, int numExpressions, int *outputRegisters) {
    OpCode      opCode, opCode2;
    Str         token;
    int         tokenType;
    int         puncType;
    int         reg;
    int         parenType;
    int         parenInfo[256];
    bool        next;
    bool        unary;
    int         i;

    g_opCodeStackPointer = -1;
    g_operandStackPointer = -1;
    
    ExprParseState lastParseState = EmptyState;
    int parenthese = 0;
    int sqbracket = 0;
    int index = 0;
    
    while (1) {
        g_tempRegisterUseCount = 0;

        while (lexer.ReadToken(&token, false)) {
            tokenType = lexer.GetTokenType();
            if (tokenType & TT_PUNCTUATION) {
                puncType = lexer.GetPunctuationType();
                switch (puncType) {
                case PuncType::P_PARENTHESESOPEN: // '(' 는 무조건 push
                    if (!(lastParseState & (EmptyState | OpState | BuiltInOpState | LParenState))) {
                        goto SYNTAX_ERROR;
                    }

                    parenType = lastParseState == BuiltInOpState ? OpCode_BuiltInLParen : OpCode_LParen;
                    lastParseState = LParenState;

                    assert(g_opCodeStackPointer + 1 < COUNT_OF(g_opCodeStack));
                    g_opCodeStack[++g_opCodeStackPointer] = parenType;
                    parenInfo[parenthese++] = parenType;
                    break;
                case PuncType::P_SQBRACKETOPEN: // '[' 도 무조건 push
                    if (!(lastParseState & (OperandState))) {
                        goto SYNTAX_ERROR;
                    }

                    lastParseState = OpState;

                    assert(g_opCodeStackPointer + 1 < COUNT_OF(g_opCodeStack));
                    g_opCodeStack[++g_opCodeStackPointer] = OpCode_Table;
                    sqbracket++;
                    break;
                case PuncType::P_PARENTHESESCLOSE:  // ')'
                    if (--parenthese < 0) {
                        goto SYNTAX_ERROR;
                    }

                    while (g_opCodeStack[g_opCodeStackPointer] != parenInfo[parenthese]) {
                        EvaluateStack();
                    }
                    g_opCodeStackPointer--; // '('
                    break;
                case PuncType::P_SQBRACKETCLOSE:    // ']'
                    if (--sqbracket < 0) {
                        goto SYNTAX_ERROR;
                    }
                    
                    while (g_opCodeStack[g_opCodeStackPointer] != OpCode_Table) {
                        EvaluateStack();
                    }
                    EvaluateStack();    // table 연산 evaluate
                    break;
                case PuncType::P_ADD:
                case PuncType::P_SUB:
                case PuncType::P_MUL:
                case PuncType::P_DIV:
                case PuncType::P_MOD:
                case PuncType::P_LOGIC_NOT:
                case PuncType::P_LOGIC_GREATER:
                case PuncType::P_LOGIC_GEQ:
                case PuncType::P_LOGIC_LESS:
                case PuncType::P_LOGIC_LEQ:
                case PuncType::P_LOGIC_EQ:
                case PuncType::P_LOGIC_UNEQ:
                case PuncType::P_LOGIC_AND:
                case PuncType::P_LOGIC_OR:
                    if (lastParseState & OperandState) {
                        unary = false;
                    } else {
                        if (!(puncType == P_LOGIC_NOT || puncType == P_ADD || puncType == P_SUB))
                            goto SYNTAX_ERROR;
                        unary = true;
                    }

                    opCode = PunctuationTypeToOpCode(puncType, unary);
                    if (opCode == OpCode_Invalid) {
                        goto SYNTAX_ERROR;
                    }
                                                
                    lastParseState = OpState;
                    
                    if (g_opCodeStackPointer > -1 && !unary) {
                        // 이전 opCode 과 precedence 비교
                        opCode2 = (OpCode)g_opCodeStack[g_opCodeStackPointer];
                        if (opCodeInfoTable[opCode2].precedence >= opCodeInfoTable[opCode].precedence) 
                            EvaluateStack();
                    }

                    assert(g_opCodeStackPointer + 1 < COUNT_OF(g_opCodeStack));
                    g_opCodeStack[++g_opCodeStackPointer] = opCode;
                    break;
                case PuncType::P_COMMA:
                    if (parenInfo[parenthese-1] == OpCode_BuiltInLParen) {
                        while (g_opCodeStack[g_opCodeStackPointer] != OpCode_BuiltInLParen) {
                            EvaluateStack();
                        }

                        lastParseState = OpState;   // HACK
                        continue;
                    }
                    next = true;
                    goto END_OF_EXPRESSION;
                case PuncType::P_SEMICOLON:
                    next = false;
                    goto END_OF_EXPRESSION;
                default:
                    goto SYNTAX_ERROR;
                }
            } else if (tokenType & TokenType::TT_IDENTIFIER) {
                if (!(lastParseState & (EmptyState | OpState | BuiltInOpState | LParenState))) {
                    goto SYNTAX_ERROR;
                }

                // built-in 연산자 리스트에서 찾는다
                opCode = FindBuiltInOpCode(token.c_str());
                if (opCode != OpCode_Invalid) {
                    lastParseState = BuiltInOpState;

                    g_opCodeStack[++g_opCodeStackPointer] = opCode;
                    continue;
                }
                
                // built-in 연산자가 아니라면 identifier (레지스터 or 파라미터 or 테이블) 다.
                lastParseState = OperandState;
                
                if (token.c_str()[0] == 'r') {
                    // 레지스터 이름에서 찾는다
                    const char *p = token.c_str() + 1;
                    for (i = 0; p[i]; i++) {
                        if (p[i] < '0' || p[i] > '9') {
                            break;
                        }
                    }

                    if (!p[i]) {
                        i = atoi(p);
                        if (i >= numRegisters) {
                            BE_ERRLOG(L"register 'r%i' used without having been initialized\n", i);
                        }
                        g_operandStack[++g_operandStackPointer] = MAKE_REG_OUTPUT(i);
                        continue;
                    }
                } else if (!token.Cmp("time")) {
                    // 로컬 파라미터에서 찾는다
                    const char *p = token.c_str() + 4;
                    g_operandStack[++g_operandStackPointer] = MAKE_REG_LOCALPARM(0);
                    continue;
                } else if (!token.Cmpn("parm", 4)) {
                    // 로컬 파라미터에서 찾는다
                    const char *p = token.c_str() + 4;
                    for (i = 0; p[i]; i++) {
                        if (p[i] < '0' || p[i] > '9') {
                            break;
                        }
                    }
                    
                    if (!p[i]) {
                        i = atoi(p) + 1;
                        g_operandStack[++g_operandStackPointer] = MAKE_REG_LOCALPARM(i);
                        continue;
                    }
                } else {
                    // 파라미터 리스트에서 찾는다.
                    i = data->FindParm(token.c_str());
                    if (i != -1) {
                        g_operandStack[++g_operandStackPointer] =  MAKE_REG_GLOBALPARM(i);
                        continue;
                    }
                
                    // 테이블 리스트에서 찾는다.
                    i = data->FindTable(token.c_str());
                    if (i != -1) {
                        g_operandStack[++g_operandStackPointer] =  MAKE_REG_TABLE(i);
                        continue;
                    }
                }
                
                BE_ERRLOG(L"undefined identifier '%hs'\n", token.c_str());
                return false;
            } else if (tokenType & TokenType::TT_NUMBER) {
                if (!(lastParseState & (EmptyState | OpState | BuiltInOpState | LParenState))) {
                    goto SYNTAX_ERROR;
                }

                lastParseState = OperandState;
                
                reg = GetConstantRegister((float)atof(token));
                g_operandStack[++g_operandStackPointer] = reg;
            } else if (tokenType & TokenType::TT_STRING) {
                goto SYNTAX_ERROR;
            } else {
                goto SYNTAX_ERROR;
            }
        }

        next = false;
        goto END_OF_EXPRESSION;               

SYNTAX_ERROR:
        BE_ERRLOG(L"syntax error: '%hs'\n", token.c_str());
        return false;

END_OF_EXPRESSION:
        if (lastParseState == OpState) {
            BE_ERRLOG(L"missing operand before '%hs'\n", token.c_str());
            return false;
        }

        if (parenthese) {
            BE_ERRLOG(L"missing ')' before '%hs'\n", token.c_str());
            return false;
        }

        if (sqbracket) {
            BE_ERRLOG(L"missing ']' before '%hs'\n", token.c_str());
            return false;
        }

        while (g_opCodeStackPointer > -1) {
            EvaluateStack();
        }

        GenerateInstruction(OpCode_Mov, MAKE_REG_OUTPUT(numRegisters), g_operandStack[0], 0);
        outputRegisters[index++] = MAKE_REG_OUTPUT(numRegisters++);

        if ((!next && index < numExpressions) || (next && index == numExpressions)) {
            BE_ERRLOG(L"expression does not take %i arguments\n", index);
            return false;
        }

        if (!next) {
            break;
        }

        g_opCodeStackPointer = -1;
        g_operandStackPointer = -1;

        lastParseState = EmptyState;
    }	
    
    return true;
}

int ExprChunk::GetConstantRegister(float constantValue) {
    int i;

/*	for (i = 0; i < numConstantRegisters; i++) {
        if (g_constantRegisters[i] == constantValue) {
            return MAKE_REG_CONSTANT(i);
        }
    }*/

    i = numConstantRegisters;
    g_constantRegisters[numConstantRegisters++] = constantValue;
        
    return MAKE_REG_CONSTANT(i);
}

float ExprChunk::GetValue(int reg) const {
    switch (GET_REG_TYPE(reg)) {
    case OutputRegister:
        return g_outputValues[GET_REG_INDEX(reg)];
    case TempRegister:
        return g_tempRegisters[GET_REG_INDEX(reg)];		
    case ConstantRegister:
        return constantRegisters[GET_REG_INDEX(reg)];
    case LocalParmRegister:
        return g_localParms[GET_REG_INDEX(reg)];
    case GlobalParmRegister:
        return data->GetParmValue(GET_REG_INDEX(reg));
    case TableRegister:
        BE_FATALERROR(L"ExprChunk::GetValue: Could not read from table register %i\n", reg);
    }

    return 0.0f;
}

void ExprChunk::SetValue(int reg, float value) {
    switch (GET_REG_TYPE(reg)) {
    case OutputRegister:
        g_outputValues[GET_REG_INDEX(reg)] = value;
        break;
    case TempRegister:
        g_tempRegisters[GET_REG_INDEX(reg)] = value;
        break;
    case ConstantRegister:
    case LocalParmRegister:
    case GlobalParmRegister:
    case TableRegister:
        BE_FATALERROR(L"ExprChunk::SetValue: Could not write to register %i\n", reg);
        break;
    }
}

void ExprChunk::Evaluate(const float localParms[MAX_EXPR_LOCALPARMS], float *outputValues) {
    g_localParms = localParms;
    g_outputValues = outputValues;

    for (int i = 0; i < numInstructions; i++) {
        Instruction instruction = instructions[i];

        switch (instruction.opCode) {
        case OpCode_Table:
            SetValue(instruction.dest, data->GetTableValue(instruction.src0, GetValue(instruction.src1)));
            break;
        case OpCode_Add:
            SetValue(instruction.dest, GetValue(instruction.src0) + GetValue(instruction.src1));
            break;
        case OpCode_Subtract:
            SetValue(instruction.dest, GetValue(instruction.src0) - GetValue(instruction.src1));
            break;
        case OpCode_Multiply:
            SetValue(instruction.dest, GetValue(instruction.src0) * GetValue(instruction.src1));
            break;
        case OpCode_Divide:
            SetValue(instruction.dest, GetValue(instruction.src0) / GetValue(instruction.src1));
            break;
        case OpCode_Mod:
            SetValue(instruction.dest, (float)((int)GetValue(instruction.src0) % (int)GetValue(instruction.src1)));
            break;	
        case OpCode_Not:
            SetValue(instruction.dest, !(int)GetValue(instruction.src0) ? 1.0f : 0.0f);
            break;
        case OpCode_Plus:
            SetValue(instruction.dest, +GetValue(instruction.src0));
            break;
        case OpCode_Minus:
            SetValue(instruction.dest, -GetValue(instruction.src0));
            break;
        case OpCode_Abs:
            SetValue(instruction.dest, Math::Fabs(GetValue(instruction.src0)));
            break;
        case OpCode_Floor:
            SetValue(instruction.dest, Math::Floor(GetValue(instruction.src0)));
            break;
        case OpCode_Ceil:
            SetValue(instruction.dest, Math::Ceil(GetValue(instruction.src0)));
            break;
        case OpCode_Fract:
            SetValue(instruction.dest, GetValue(instruction.src0) - Math::Floor(GetValue(instruction.src0)));
            break;
        case OpCode_Sqrt:
            SetValue(instruction.dest, Math::Sqrt(GetValue(instruction.src0)));
            break;
        case OpCode_InvSqrt:
            SetValue(instruction.dest, Math::InvSqrt(GetValue(instruction.src0)));
            break;
        case OpCode_Min:
            SetValue(instruction.dest, Min(GetValue(instruction.src0), GetValue(instruction.src1)));
            break;
        case OpCode_Max:
            SetValue(instruction.dest, Max(GetValue(instruction.src0), GetValue(instruction.src1)));
            break;
        case OpCode_Pow:
            SetValue(instruction.dest, Math::Pow(GetValue(instruction.src0), GetValue(instruction.src1)));
            break;
        case OpCode_Exp:
            SetValue(instruction.dest, Math::Exp(GetValue(instruction.src0)));
            break;
        case OpCode_Log:
            SetValue(instruction.dest, Math::Log(GetValue(instruction.src0)));
            break;
        case OpCode_Sin:
            SetValue(instruction.dest, Math::Sin(DEG2RAD(Math::AngleNormalize360(GetValue(instruction.src0)))));
            break;
        case OpCode_Cos:
            SetValue(instruction.dest, Math::Cos(DEG2RAD(Math::AngleNormalize360(GetValue(instruction.src0)))));
            break;
        case OpCode_Tan:
            SetValue(instruction.dest, Math::Tan(DEG2RAD(Math::AngleNormalize360(GetValue(instruction.src0)))));
            break;
        case OpCode_ASin:
            SetValue(instruction.dest, Math::ASin(GetValue(instruction.src0)));
            break;
        case OpCode_ACos:
            SetValue(instruction.dest, Math::ACos(GetValue(instruction.src0)));
            break;
        case OpCode_ATan:
            SetValue(instruction.dest, Math::ATan(GetValue(instruction.src0)));
            break;
        case OpCode_GreaterThan:
            SetValue(instruction.dest, GetValue(instruction.src0) > GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_GreaterEqual:
            SetValue(instruction.dest, GetValue(instruction.src0) >= GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_LessThan:
            SetValue(instruction.dest, GetValue(instruction.src0) < GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_LessEqual:
            SetValue(instruction.dest, GetValue(instruction.src0) <= GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_Equal:
            SetValue(instruction.dest, GetValue(instruction.src0) == GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_NotEqual:
            SetValue(instruction.dest, GetValue(instruction.src0) != GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_And:
            SetValue(instruction.dest, GetValue(instruction.src0) && GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_Or:
            SetValue(instruction.dest, GetValue(instruction.src0) || GetValue(instruction.src1) ? 1.0f : 0.0f);
            break;
        case OpCode_Mov:
            SetValue(instruction.dest, GetValue(instruction.src0));
            break;
        default:
            assert(0);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------

void ExprData::Clear() {
    for (int i = 0; i < numTables; i++) {
        SAFE_DELETE_ARRAY(tables[i].elements);
    }

    numTables = 0;
    numParms = 0;
}

int ExprData::FindParm(const char *name) const {
    for (int i = 0; i < numParms; i++) {
        if (!Str::Cmp(parms[i].name, name)) {
            return i;
        }
    }

    return -1;
}

int ExprData::FindTable(const char *name) const {
    for (int i = 0; i < numTables; i++) {
        if (!Str::Cmp(tables[i].name, name)) {
            return i;
        }
    }

    return -1;
}

void ExprData::SetParm(const char *name, float value) {
    for (int i = 0; i < numParms; i++) {
        if (!Str::Cmp(parms[i].name, name)) {
            parms[i].value = value;
            return;
        }
    }

    strcpy(parms[numParms].name, name);
    parms[numParms].value = value;
    numParms++;
}

void ExprData::SetTable(const char *name, int numElements, const float *elements, bool snap, bool wrap) {
    for (int i = 0; i < numTables; i++) {
        if (!Str::Cmp(tables[i].name, name)) {
            if (tables[i].numElements != numElements) {
                BE_FATALERROR(L"ExprData::SetTable: table elements(%i) != numElements(%i)", tables[i].numElements, numElements);
                return;
            }

            memcpy(tables[i].elements, elements, numElements * sizeof(float));
            tables[i].snap = snap;
            tables[i].wrap = wrap;
            return;
        }
    }

    strcpy(tables[numTables].name, name);
    tables[numTables].numElements = numElements;
    tables[numTables].elements = new float[numElements];
    memcpy(tables[numTables].elements, elements, numElements * sizeof(float));
    tables[numTables].snap = snap;
    tables[numTables].wrap = wrap;
    numTables++;
}

float ExprData::GetParmValue(int reg) const {
    int i = GET_REG_INDEX(reg);
    if (i >= numParms) {
        BE_FATALERROR(L"ExprData::GetParmValue: %i > numParms - 1 (%i)", i, numParms);
    }

    return parms[i].value;
}

float ExprData::GetTableValue(int reg, float findex) const {
    float output;
    int i0, i1;

    int i = GET_REG_INDEX(reg);
    if (i >= numTables) {
        BE_FATALERROR(L"ExprData::GetTableValue: %i > numTables - 1 (%i)", i, numTables);
    }

    const Table *table = &tables[i];

    if (table->snap) {
        i0 = (int)Math::Floor(findex);
        if (table->wrap) {
            Wrap(i0, 0, table->numElements - 1);
        } else {
            Clamp(i0, 0, table->numElements - 1);
        }

        output = table->elements[i0];
    } else {
        i0 = (int)Math::Floor(findex);
        float fraction = findex - (float)i0;
        if (table->wrap) {
            Wrap(i0, 0, table->numElements - 1);
        } else {
            Clamp(i0, 0, table->numElements - 1);
        }

        i1 = i0 + 1;
        if (table->wrap) {
            Wrap(i1, 0, table->numElements - 1);
        } else {
            Clamp(i1, 0, table->numElements - 1);
        }
        
        output = table->elements[i0] + (table->elements[i1] - table->elements[i0]) * fraction;
    }
    
    return output;
}

BE_NAMESPACE_END
