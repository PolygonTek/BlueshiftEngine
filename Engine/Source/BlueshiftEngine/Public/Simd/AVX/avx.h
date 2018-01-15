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

#if defined(__AVX__)
#include <immintrin.h>
#else
#include "../immintrin_emu.h"
#endif

#include "avxb.h"
#include "avxi.h"
#include "avxf.h"

#define BEGIN_ITERATE_AVXB(valid_i, id_o) { \
  int _valid_t = movemask(valid_i); \
  while (_valid_t) { \
    int id_o = __bsf(_valid_t); \
    _valid_t = __btc(_valid_t, id_o);
#define END_ITERATE_AVXB } }
