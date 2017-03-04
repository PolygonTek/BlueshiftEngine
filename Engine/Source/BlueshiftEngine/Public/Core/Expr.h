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

//--------------------------------------------------------------------------------------------------
//
// Expression Data & Chunk
//
// 표현식의 코드와 데이터를 포함한 자료형.
// 간단한 연산 및 테이블연산을 지원한다.
//--------------------------------------------------------------------------------------------------

#define MAX_EXPR_TABLES         256
#define MAX_EXPR_GLOBALPARMS    64
#define MAX_EXPR_LOCALPARMS     16

// 상위 16 비트가 register type
#define GET_REG_TYPE(r)         (((r) & 0xFFFF0000) >> 16)
#define GET_REG_INDEX(r)        ((r) & ~0xFFFF0000)

BE_NAMESPACE_BEGIN

class BE_API ExprData {
    friend class ExprChunk;

public:
    struct Parm {
        char            name[32];
        float           value;
    };

    struct Table {
        char            name[32];
        int             numElements;
        float *         elements;
        bool            snap;
        bool            wrap;
    };

    ExprData();

    void                Clear();

    void                SetParm(const char *name, float value);
    void                SetTable(const char *name, int numElements, const float *elements, bool snap, bool wrap);

    float               GetParmValue(int reg) const;
    float               GetTableValue(int reg, float findex) const;

private:
    int                 FindParm(const char *name) const;
    int                 FindTable(const char *name) const;

    int                 numParms;
    Parm                parms[MAX_EXPR_GLOBALPARMS];

    int                 numTables;
    Table               tables[MAX_EXPR_TABLES];
};

BE_INLINE ExprData::ExprData() {
    numParms = 0;
    numTables = 0;
}

class BE_API ExprChunk {
public:
    enum RegisterType {
        InvalidRegister,
        OutputRegister,
        TempRegister,
        ConstantRegister,
        TableRegister,
        GlobalParmRegister,
        LocalParmRegister
    };

    enum OpCode {
        OpCode_Invalid,
        OpCode_LParen,          // (
        OpCode_BuiltInLParen,   // (
        OpCode_Table,           // [
        OpCode_Not,             // ! (nnary)
        OpCode_Plus,            // + (unary)
        OpCode_Minus,           // - (unary)
        OpCode_Abs,             // abs
        OpCode_Floor,           // floor
        OpCode_Ceil,            // ceil
        OpCode_Fract,           // fract
        OpCode_Sqrt,            // sqrt
        OpCode_InvSqrt,         // inverse sqrt
        OpCode_Min,             // min
        OpCode_Max,             // max
        OpCode_Pow,             // power
        OpCode_Exp,             // exponent
        OpCode_Log,             // log
        OpCode_Sin,             // sine
        OpCode_Cos,             // cosine
        OpCode_Tan,             // tangent
        OpCode_ASin,            // arc sine
        OpCode_ACos,            // arc cosine
        OpCode_ATan,            // arc tangent
        OpCode_Multiply,        // *
        OpCode_Divide,          // /
        OpCode_Mod,             // %
        OpCode_Add,             // +
        OpCode_Subtract,        // -
        OpCode_GreaterThan,     // >
        OpCode_GreaterEqual,    // >=
        OpCode_LessThan,        // <
        OpCode_LessEqual,       // <=
        OpCode_Equal,           // ==
        OpCode_NotEqual,        // !=
        OpCode_And,             // &&
        OpCode_Or,              // ||
        OpCode_Mov              // =
    };

    struct Instruction {
        OpCode          opCode;
        int             src0, src1;
        int             dest;
    };

    ExprChunk();
    
    void                Init(ExprData *data = nullptr);
    void                Free();

    bool                ParseExpressions(Lexer &lexer, int numExpressions, int *registers);
    bool                ParseExpressions(const char *text, int numExpressions, int *registers);
    void                Finish();

    int                 NumRegisters() const { return numRegisters; }
    
                        // localParms[0] is a 'time' and others are 'parm0', 'parm1', 'parm2', .. 
    void                Evaluate(const float localParms[MAX_EXPR_LOCALPARMS], float *outputValues);

private:
    int                 GetConstantRegister(float constantValue);

    void                GenerateInstruction(OpCode opCode, int dest, int src0, int src1);
    void                EvaluateStack();

    float               GetValue(int reg) const;
    void                SetValue(int reg, float value);	

    int                 numRegisters;
    int                 numConstantRegisters;
    int                 numInstructions;
    float *             constantRegisters;
    Instruction *       instructions;

    ExprData *          data;
};

BE_INLINE ExprChunk::ExprChunk() {
    Init();
}

BE_NAMESPACE_END
