#include "BlueshiftEngine.h"
#include "Test.h"

struct NumberTokenCase {
    const char *text;
    const char *token;
    float value;
    int precision;
};

static void TestScientificNotation() {
    static const NumberTokenCase cases[] = {
        { "1e3",       "1e3",     1000.0f, BE1::Lexer::TokenType::DoublePrecision },
        { "0e3",       "0e3",        0.0f, BE1::Lexer::TokenType::DoublePrecision },
        { "0012e2",    "0012e2",  1200.0f, BE1::Lexer::TokenType::DoublePrecision },
        { "1.0E3",     "1.0E3",   1000.0f, BE1::Lexer::TokenType::DoublePrecision },
        { ".5e+2F",    ".5e+2",     50.0f, BE1::Lexer::TokenType::SinglePrecision },
        { "1.E2L",     "1.E2",     100.0f, BE1::Lexer::TokenType::ExtendedPrecision }
    };

    for (const NumberTokenCase &testCase : cases) {
        BE1::Lexer lexer(testCase.text, BE1::Str::Length(testCase.text), "scientific-notation", BE1::Lexer::Flag::NoErrors);
        BE1::Str token;
        assert(lexer.ReadToken(&token));
        assert(token == testCase.token);
        assert((lexer.GetTokenType() & (BE1::Lexer::TokenType::Number | BE1::Lexer::TokenType::Float)) ==
            (BE1::Lexer::TokenType::Number | BE1::Lexer::TokenType::Float));
        assert((lexer.GetTokenType() & testCase.precision) != 0);
        assert((float)atof(token.c_str()) == testCase.value);
        assert(lexer.EndOfFile());
    }
}

static void TestMalformedScientificNotation() {
    static const char *cases[] = {
        "1e",
        "1e+",
        "1.0E-"
    };

    for (const char *text : cases) {
        BE1::Lexer lexer(text, BE1::Str::Length(text), "malformed-scientific-notation", BE1::Lexer::Flag::NoErrors);
        BE1::Str token;
        assert(!lexer.ReadToken(&token));
    }
}

void Test::TestLexer() {
    TestScientificNotation();
    TestMalformedScientificNotation();
}
