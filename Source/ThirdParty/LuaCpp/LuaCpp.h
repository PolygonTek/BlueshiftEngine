#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1900 // before MSVS-14 CTP1
#define constexpr const
#endif

// --- Lua ---
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#if USE_LUAJIT
    #include "luajit.h"
#endif
}

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

