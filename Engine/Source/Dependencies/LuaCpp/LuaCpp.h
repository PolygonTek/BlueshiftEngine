#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1900 // before MSVS-14 CTP1
#define constexpr const
#endif

// --- Lua ---
#if USE_LUAJIT
#include "luaJIT/src/lua.hpp"
#else
extern "C" {
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}
#endif

// --- Cpp ---
#include <iostream>
#include <cstdint>
#include <type_traits>
#include <string>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <memory>
#include <functional>
#include <exception>
#include <cassert>



#ifdef __ANDROID__

namespace std
{
	template<typename _String, typename _CharT = typename _String::value_type>
	_String
		__to_xstring(int(*__convf) (_CharT*, std::size_t, const _CharT*,
			__builtin_va_list), std::size_t __n,
			const _CharT* __fmt, ...)
	{
		// XXX Eventually the result will be constructed in place in
		// the C++0x string, likely with the help of internal hooks.
		_CharT* __s = static_cast<_CharT*>(__builtin_alloca(sizeof(_CharT)
			* __n));

		__builtin_va_list __args;
		__builtin_va_start(__args, __fmt);

		const int __len = __convf(__s, __n, __fmt, __args);

		__builtin_va_end(__args);

		return _String(__s, __s + __len);
	}


	inline string
		to_string(int __val)
	{
		return __to_xstring<string>(&vsnprintf, 4 * sizeof(int),
			"%d", __val);
	}

	inline string
		to_string(unsigned __val)
	{
		return __to_xstring<string>(&vsnprintf,
			4 * sizeof(unsigned),
			"%u", __val);
	}

	inline string
		to_string(long __val)
	{
		return __to_xstring<string>(&vsnprintf, 4 * sizeof(long),
			"%ld", __val);
	}

	inline string
		to_string(unsigned long __val)
	{
		return __to_xstring<string>(&vsnprintf,
			4 * sizeof(unsigned long),
			"%lu", __val);
	}

	inline string
		to_string(long long __val)
	{
		return __to_xstring<string>(&vsnprintf,
			4 * sizeof(long long),
			"%lld", __val);
	}

	inline string
		to_string(unsigned long long __val)
	{
		return __to_xstring<string>(&vsnprintf,
			4 * sizeof(unsigned long long),
			"%llu", __val);
	}

	inline string
		to_string(float __val)
	{
		const int __n =
			__gnu_cxx::__numeric_traits<float>::__max_exponent10 + 20;
		return __to_xstring<string>(&vsnprintf, __n,
			"%f", __val);
	}

	inline string
		to_string(double __val)
	{
		const int __n =
			__gnu_cxx::__numeric_traits<double>::__max_exponent10 + 20;
		return __to_xstring<string>(&vsnprintf, __n,
			"%f", __val);
	}

	inline string
		to_string(long double __val)
	{
		const int __n =
			__gnu_cxx::__numeric_traits<long double>::__max_exponent10 + 20;
		return __to_xstring<string>(&vsnprintf, __n,
			"%Lf", __val);
	}
}
#endif

// --- LuaCpp ---
#include "Traits.h"
#include "ExceptionTypes.h"
#include "MetatableRegistry.h"
#include "LuaStack.h"
#include "ExceptionHandler.h"
#include "Util.h"
#include "LuaRef.h"
#include "ResourceHandler.h"
#include "function.h"
#include "BaseFunc.h"
#include "Func.h"
#include "ObjFunc.h"
#include "Obj.h"
#include "ClassFunc.h"
#include "Ctor.h"
#include "Dtor.h"
#include "Class.h"
#include "Registry.h"
#include "Reference.h"
#include "Pointer.h"
#include "Selector.h"
#include "Searcher.h"
#include "Module.h"
#include "State.h"
#include "Tuple.h"

