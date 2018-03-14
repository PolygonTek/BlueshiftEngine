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

#include "BlueshiftEngine.h"
#include "LuaCpp/LuaCpp.h"
#include "TestLua.h"

static void TestLuaFunction(LuaCpp::State &lua) {
    // Lua function that returns single value
    lua(R"(
        function sum(x, y) 
            return x + y 
        end
    )");

    int ret = lua["sum"](1, 2);
    assert(ret == 3);

    // Lua function that returns multiple values
    lua(R"(
        function sum_sub(x, y) 
            return x + y, x - y 
        end
    )");

    float sum, sub;
    LuaCpp::tie(sum, sub) = lua["sum_sub"](1.5f, 1.5f);
    assert(sum == 3.0f && sub == 0.0f);
}

static int sum(int a, int b) {
    return a + b;
}

static void TestCppFunction(LuaCpp::State &lua) {
    // Register function pointer to Lua
    lua["sum"].SetFunc(::sum);
    lua("ret = sum(1, 2)");
    assert(lua["ret"] == 3);

    // Register lambda to Lua
    lua["sub"].SetFunc([](int a, int b) {
        return a - b;
    });
    lua("ret = sub(1, 2)");
    assert(lua["ret"] == -1);
}

class Base {
public:
    int Func1() { return 1; }
    int Func2() { return 2; }
    float x, y, z;
};

class Derived : public Base {
public:
    int Func2() { return 20; }
};

static void TestInheritance(LuaCpp::State &lua) {
    auto luaBase = lua["Base"];
    luaBase.SetClass<Base>();
    luaBase.AddClassCtor<Base>();
    luaBase.AddClassMembers<Base>(
        "x", &Base::x,
        "y", &Base::y,
        "z", &Base::z,
        "func1", &Base::Func1,
        "func2", &Base::Func2);

    auto luaDerived = lua["Derived"];
    luaDerived.SetClass<Derived>(luaBase);
    luaDerived.AddClassCtor<Derived>();
    luaDerived.AddClassMembers<Derived>(
        "func2", &Derived::Func2);

    lua(R"(
        base = Base()
        b1 = base:func1()
        b2 = base:func2()

        derived = Derived()
        d1 = derived:func1() -- call base function
        d2 = derived:func2() -- call overrided function
    )");

    assert(lua["b1"] == 1);
    assert(lua["b2"] == 2);
    assert(lua["d1"] == 1);
    assert(lua["d2"] == 20);
}

class VirtualBase {
public:
    virtual int Func1() = 0;
    virtual int Func2() { return 2; }
    float x, y, z;
};

class VirtualDerived : public VirtualBase {
public:
    virtual int Func1() override { return 10; }
};

static void TestPolymorphism(LuaCpp::State &lua) {
    auto luaVirtualBase = lua["VirtualBase"];
    luaVirtualBase.SetClass<VirtualBase>();
    luaVirtualBase.AddClassCtor<VirtualBase>();
    luaVirtualBase.AddClassMembers<VirtualBase>(
        "x", &VirtualBase::x,
        "y", &VirtualBase::y,
        "z", &VirtualBase::z,
        "func1", &VirtualBase::Func1,
        "func2", &VirtualBase::Func2);

    auto luaVirtualDerived = lua["VirtualDerived"];
    luaVirtualDerived.SetClass<VirtualDerived>(luaVirtualBase);
    luaVirtualDerived.AddClassCtor<VirtualDerived>();
    luaVirtualDerived.AddClassMembers<VirtualDerived>(
        "func1", &VirtualDerived::Func1);

    lua(R"(
        virtual_derived = VirtualDerived()
        p1 = virtual_derived:func1() -- call overrided function
        p2 = virtual_derived:func2() -- call base function
    )");

    assert(lua["p1"] == 10);
    assert(lua["p2"] == 2);
}

static void TestObject(LuaCpp::State &lua) {
    Base base = { 1, 2, 3 };
    lua["TEST"]["base"].SetObj(base);
    lua["TEST"]["base"].AddObjMembers(base,
        "x", &Base::x,
        "y", &Base::y,
        "z", &Base::z
    );
    lua("print('TEST.base:y() == ', TEST.base:y())");

    lua("TEST.base.testfunc = function() print('hello') end");
    if (lua["TEST"]["base"]["testfunc"].IsFunction()) {
        lua["TEST"]["base"]["testfunc"]();
    }
}

struct XXX {
    int x;
};

struct YYY {
    int y;
};

static void TestCallByReference(LuaCpp::State &lua) {
    lua["XXX"].SetClass<XXX>();
    lua["XXX"].AddClassMembers<XXX>("x", &XXX::x);

    lua["YYY"].SetClass<YYY>();
    lua["YYY"].AddClassMembers<YYY>("y", &YYY::y);

    lua(R"(
        function exchange_value(x, y)
            local tmp = x:x()
            x:set_x(y:y())
            y:set_y(tmp)
        end
    )");
    
    XXX x = { 1 };
    YYY y = { 2 };
    lua["exchange_value"](x, y); // call by lvalue reference
    assert(x.x == 2 && y.y == 1);

    lua["exchange_value"](x, YYY{ 3 });  // call by lvaue / rvalue reference
    assert(x.x == 3);
}

static void TestVec3(LuaCpp::State &lua) {
    auto luaVec3 = lua["Vec3"];
    luaVec3.SetClass<BE1::Vec3>();
    luaVec3.AddClassCtor<BE1::Vec3, float, float, float>("__call");
    luaVec3.AddClassMembers<BE1::Vec3>(
        "x", &BE1::Vec3::x,
        "y", &BE1::Vec3::y,
        "z", &BE1::Vec3::z
    );    
    luaVec3.AddClassMembers<BE1::Vec3>(
        "at", static_cast<float &(BE1::Vec3::*)(int)>(&BE1::Vec3::At), // index start from zero
        "set", &BE1::Vec3::Set,
        "equals", static_cast<bool(BE1::Vec3::*)(const BE1::Vec3 &, const float) const>(&BE1::Vec3::Equals),
        "length", &BE1::Vec3::Length,
        "length_squared", &BE1::Vec3::LengthSqr,
        "distance", &BE1::Vec3::Distance,
        "distance_squared", &BE1::Vec3::DistanceSqr,
        "dot", &BE1::Vec3::Dot,
        "cross", &BE1::Vec3::Cross
    );
    luaVec3.AddClassMembers<BE1::Vec3>(
        "__tostring", static_cast<const char *(BE1::Vec3::*)() const>(&BE1::Vec3::ToString),
        "__unm", static_cast<BE1::Vec3(BE1::Vec3::*)() const>(&BE1::Vec3::operator-),
        "__add", &BE1::Vec3::Add,
        "__sub", &BE1::Vec3::Sub,
        "__mul", static_cast<BE1::Vec3(BE1::Vec3::*)(const float) const>(&BE1::Vec3::operator*),
        "__div", static_cast<BE1::Vec3(BE1::Vec3::*)(const float) const>(&BE1::Vec3::operator/),
        "__eq", static_cast<bool(BE1::Vec3::*)(const BE1::Vec3 &) const>(&BE1::Vec3::operator==)
    );

    // Register static member function (like global function)
    luaVec3["compute_bitangent_sign"].SetFunc(BE1::Vec3::ComputeBitangentSign);

    // Register static member variable (like global variable)
    luaVec3["zero"] = BE1::Vec3::zero;
    luaVec3["one"] = BE1::Vec3::one;

    // ---- TEST CODE ----
    lua(R"(
        --print(getmetatable(Vec3.one))

        a = Vec3(1, 0, 0)
        b = Vec3(0, 1, 0)
        c = Vec3(0, 0, 0)
        c = a:cross(b)
        print(a)

        sign = Vec3.compute_bitangent_sign(a, b, c) -- call static member function
        
        a = a + b + c -- operator overloading

        is_same = a == Vec3.one

        print(a:length())
    )");
    assert(lua["is_same"] == true);

    // get value by reference
    BE1::Vec3 &a = lua["a"];
    assert(a == BE1::Vec3::one);
}

static void TestTableEnumeration(LuaCpp::State &lua) {
    lua(R"(
        properties = {
            x = { name = "x" },
            y = { name = "y" },
            z = { name = "z" }
        }
    )");

    assert(lua["properties"].Name() == "properties");
    lua["properties"].Enumerate([](LuaCpp::Selector &selector) {
        printf("%s\n", selector.Name().c_str());
        printf("%s\n", (const char *)selector["name"]);
        //selector["name"] = "hello";
    });

    lua(R"(
        print(properties.x.name)
        print(properties.y.name)
        print(properties.z.name)
    )");
}

static void TestModule(LuaCpp::State &lua) {
    lua.RegisterSearcher([&lua](const std::string &name) {
        printf(name.c_str());
        return true;
    });

    lua.RegisterModule("blueshift.math", [](LuaCpp::Module &module) {
        module["Vec3"].SetClass<BE1::Vec3>();
        module["Vec3"].AddClassCtor<BE1::Vec3, float, float, float>("__call");
        module["Vec3"].AddClassMembers<BE1::Vec3>(
            "x", &BE1::Vec3::x,
            "y", &BE1::Vec3::y,
            "z", &BE1::Vec3::z
        );

        module["sub"].SetFunc([](int a, int b) {
            return a - b;
        });
    });

    lua.RunBuffer("TestModule", R"(
        --print(package.path) -- default path
        --print(package.cpath) -- default cpath
        print(_ENV, _G) -- _ENV is not defined by Lua 5.1
        local haha = require "haha"
        local math = require "blueshift.math"
        local math2 = require "blueshift.math"
        local Vec3 = math.Vec3
        --print(type(Vec3))
        local v = Vec3(1, 2, 3)
        print(v:x(), v:y(), v:z())
    )", 0, "sandbox");
}

static void TestCompile(LuaCpp::State &lua) {
    std::string byteCode;
    bool success = lua.Compile("test_compile", R"(
        function sum3(x, y, z) 
            return x + y + z
        end
        print("This is compiled code")
    )", byteCode);
    if (success) {
        lua.RunBuffer("test_compile", byteCode.data(), byteCode.size(), "sandbox");
        int s = lua["sandbox"]["sum3"](1, 2, 3);
        assert(s == 6);
    }
}

void TestLua() {
    LuaCpp::State lua(true);

    lua.HandleExceptionsWith([](int status, std::string msg, std::exception_ptr exception) {
        const char *statusStr = "";
        switch (status) {
        case LUA_ERRRUN:
            statusStr = "LUA_ERRRUN";
            break;
        case LUA_ERRSYNTAX:
            statusStr = "LUA_ERRSYNTAX";
            break;
        case LUA_ERRMEM:
            statusStr = "LUA_ERRMEM";
            break;
#if LUA_VERSION_NUM >= 502
        case LUA_ERRGCMM:
            statusStr = "LUA_ERRGCMM";
            break;
#endif
        case LUA_ERRERR:
            statusStr = "LUA_ERRERR";
            break;
        }
        BE_ERRLOG(L"%hs - %hs\n", statusStr, msg.c_str());
    });
    
    TestLuaFunction(lua);
    TestCppFunction(lua);
    TestInheritance(lua);
    TestPolymorphism(lua);
    TestObject(lua);
    TestCallByReference(lua);
    TestVec3(lua);
    TestTableEnumeration(lua);
    TestModule(lua);
    TestCompile(lua);
}