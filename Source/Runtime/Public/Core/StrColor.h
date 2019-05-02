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

BE_NAMESPACE_BEGIN

// indexes are based on Vec4Color::table

// color escape macro
#define C_COLOR_INDEX(c)                ((c) - '0')

// color escape macro
#define UC_COLOR_INDEX(c)               ((c) - U'0')

// color escape character
constexpr int C_COLOR_ESCAPE            = '^';
constexpr int C_COLOR_DEFAULT           = '0';
constexpr int C_COLOR_RED               = '1';
constexpr int C_COLOR_GREEN             = '2';
constexpr int C_COLOR_YELLOW            = '3';
constexpr int C_COLOR_BLUE              = '4';
constexpr int C_COLOR_CYAN              = '5';
constexpr int C_COLOR_MAGENTA           = '6';
constexpr int C_COLOR_WHITE             = '7';
constexpr int C_COLOR_GRAY              = '8';
constexpr int C_COLOR_BLACK             = '9';
constexpr int C_COLOR_ORANGE            = ':';
constexpr int C_COLOR_PINK              = ';';
constexpr int C_COLOR_LAWN              = '<';
constexpr int C_COLOR_MINT              = '=';
constexpr int C_COLOR_VIOLET            = '>';
constexpr int C_COLOR_TEAL              = '?';

// color escape character in unicode
constexpr uint32_t UC_COLOR_ESCAPE      = U'^';
constexpr uint32_t UC_COLOR_DEFAULT     = U'0';
constexpr uint32_t UC_COLOR_RED         = U'1';
constexpr uint32_t UC_COLOR_GREEN       = U'2';
constexpr uint32_t UC_COLOR_YELLOW      = U'3';
constexpr uint32_t UC_COLOR_BLUE        = U'4';
constexpr uint32_t UC_COLOR_CYAN        = U'5';
constexpr uint32_t UC_COLOR_MAGENTA     = U'6';
constexpr uint32_t UC_COLOR_WHITE       = U'7';
constexpr uint32_t UC_COLOR_GRAY        = U'8';
constexpr uint32_t UC_COLOR_BLACK       = U'9';
constexpr uint32_t UC_COLOR_ORANGE      = U':';
constexpr uint32_t UC_COLOR_PINK        = U';';
constexpr uint32_t UC_COLOR_LAWN        = U'<';
constexpr uint32_t UC_COLOR_MINT        = U'=';
constexpr uint32_t UC_COLOR_VIOLET      = U'>';
constexpr uint32_t UC_COLOR_TEAL        = U'?';

// color escape string
#define S_COLOR_DEFAULT                 "^0"
#define S_COLOR_RED                     "^1"
#define S_COLOR_GREEN                   "^2"
#define S_COLOR_YELLOW                  "^3"
#define S_COLOR_BLUE                    "^4"
#define S_COLOR_CYAN                    "^5"
#define S_COLOR_MAGENTA                 "^6"
#define S_COLOR_WHITE                   "^7"
#define S_COLOR_GRAY                    "^8"
#define S_COLOR_BLACK                   "^9"
#define S_COLOR_ORANGE                  "^:"
#define S_COLOR_PINK                    "^;"
#define S_COLOR_LAWN                    "^<"
#define S_COLOR_MINT                    "^="
#define S_COLOR_VIOLET                  "^>"
#define S_COLOR_TEAL                    "^?"

BE_NAMESPACE_END
