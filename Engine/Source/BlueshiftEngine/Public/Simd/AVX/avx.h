#ifndef _B_AVX_H_INCLUDED_
#define _B_AVX_H_INCLUDED_

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

#endif // _B_AVX_H_INCLUDED_
