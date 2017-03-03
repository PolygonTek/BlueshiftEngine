#include "LuaCpp.h"

namespace LuaCpp {

std::string Selector::_fullname() const {
    std::string fullname;
    if (_module) {
        fullname += _module->_name;
        fullname += ".";
    }
    fullname += _name;
    return fullname;
}

void Selector::_call_functor(int num_ret) const {
    if (!_functor_active) {
        return;
    }
    _functor_active = false;
    // install handler, and swap(handler, function) on lua stack
    int handler_index = SetErrorHandler(_l);
    int func_index = handler_index - 1;
#if LUA_VERSION_NUM >= 502
    lua_pushvalue(_l, func_index);
    lua_copy(_l, handler_index, func_index);
    lua_replace(_l, handler_index);
#else
    lua_pushvalue(_l, func_index);
    lua_pushvalue(_l, handler_index);
    lua_replace(_l, func_index);
    lua_replace(_l, handler_index);
#endif
    // pushes all arguments
    for (auto const &arg : _functor_arguments) {
        arg.Push(_l);
    }
    // call lua function with error handler
    auto const statusCode = lua_pcall(_l, (int)_functor_arguments.size(), num_ret, handler_index - 1);

    // remove error handler
    lua_remove(_l, handler_index - 1);

    if (statusCode != 0) {
        _exception_handler->Handle_top_of_stack(statusCode, _l);
    }
}

}