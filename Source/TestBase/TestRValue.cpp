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
#include "TestRValue.h"

class Number {
public:
    // default ctor
    Number() {
        BE_LOG("Called default ctor\n");
        n = 0;
    }
    // dtor
    ~Number() {
        BE_LOG("Called dtor\n");
    }
    // ctor with number
    Number(int n) {
        BE_LOG("Called ctor with number\n");
        this->n = n;
    }
    // copy ctor
    Number(const Number &rhs) {
        BE_LOG("Called copy-ctor\n");
        this->n = rhs.n;
    }
    // move ctor
    Number(Number &&rhs) noexcept {
        BE_LOG("Called move-ctor\n");
        BE1::Swap(n, rhs.n);
    }
    // copy assignment: const lvalue refernce 이므로 우측이 lvalue 이거나 rvalue 일 때 호출된다.
    Number &operator=(const Number &rhs) {
        BE_LOG("Called copy assignment\n");
        this->n = rhs.n;
        return *this;
    }
    // move assignment: 우측이 rvalue 일 경우에 윗 함수보다 우선적이다.
    Number &operator=(Number &&rhs) noexcept {
        BE_LOG("Called move assignment\n");
        BE1::Swap(n, rhs.n);
        return *this;
    }
    // 이름 없는 임시 객체를 만들어서 리턴한다.
    // RVO 가 적용되면 받는 쪽(caller)에서 ctor 나 assignment operator 가 따로 호출되지 않는다.
    Number RVO(const Number &rhs) const {
        BE_LOG("Called RVO\n");
        return Number(n + rhs.n);
    }
    // 이름 있는 임시 객체를 만들어서 리턴한다.
    // NRVO 가 적용되면 받는 쪽(caller)에서 ctor 나 assignment operator 가 따로 호출되지 않는다.
    Number NRVO(const Number &rhs) const {
        BE_LOG("Called NRVO\n");
        Number named(n + rhs.n);
        return named;
    }
    // +
    Number operator+(const Number &rhs) const {
        BE_LOG("Called operator+ with [*this == const]\n");
        return Number(n + rhs.n);
    }
    // - for lvalue
    Number operator-(const Number &rhs) const & { // *this 가 lvalue 이고 다른 객체와의 덧셈시 호출된다.
        BE_LOG("Called operator- with [*this == const &]\n");
        return Number(n - rhs.n); 
        
    }
    // - for rvalue
    Number &&operator-(const Number &rhs) && { // *this 가 rvalue 이고 다른 객체와의 덧셈시 호출된다.
        BE_LOG("Called operator- with [*this == &&]\n");
        n -= rhs.n;
        return std::move(*this); // this 가 임시객체 이므로, 새 임시 객체를 만들지 않고 rvalue 로서의 this 를 그대로 리턴
    }

private:
    int n;
};

void TestRValue() {
    int a = 1; // value assignment to lvalue.
    int &b = a; // lvalue reference.
    const int &c = 3; // const lvalue reference can reference rvalue.
    int &&d = 4; // rvalue reference. rvalue reference can't reference lvalue.
    int &&e = std::move(a); // std::move cast lvalue to rvalue type.

    BE_LOG("Number num1(1)\n");
    Number num1(1);

    BE_LOG("\nNumber num2(num1)\n");
    Number num2(num1); // Called copy-ctor.

    BE_LOG("\nNumber num3 = num2\n");
    Number num3 = num2; // Called copy-ctor (선언과 동시에 대입은 copy-ctor 를 호출한다)

    BE_LOG("\nNumber num4 = std::move(num1)\n");
    Number num4 = std::move(num1); // Called move-ctor (std::move() is same as static_cast<Number &&>(rhs))

    BE_LOG("\nnum4 = num2\n");
    num4 = num2; // copy assignment

    BE_LOG("\nnum4 = Number(1)\n");
    num4 = Number(1); // move assignment

    BE_LOG("\nNumber num5 = num2.RVO(num3)\n");
    Number num5 = num2.RVO(num3); // test RVO

    BE_LOG("\nnum5 = num2.RVO(num3)\n");
    num5 = num2.RVO(num3);

    BE_LOG("\nNumber num6 = num2.NRVO(num3)\n");
    Number num6 = num2.NRVO(num3); // test NRVO

    BE_LOG("\nnum6 = num2.NRVO(num3)\n");
    num6 = num2.NRVO(num3);

    BE_LOG("\nNumber num7 = num2 + num3\n");
    Number num7 = num2 + num3; // 임시 객체를 대입했지만 copy-ctor 는 호출되지 않는다. RVO 때문에 임시객체 자체가 리턴값을 받는 객체와 동일한 객체가 된다.

    BE_LOG("\nNumber num8 = num2 - num3\n");
    Number num8 = num2 - num3; // 임시 객체를 대입했지만 copy-ctor 는 호출되지 않는다. RVO 때문에 임시객체 자체가 리턴값을 받는 객체와 동일한 객체가 된다.

    BE_LOG("\nNumber num9 = num2 + num3 + num4 + num5\n");
    Number num9 = num2 + num3 + num4 + num5 + num6; // 임시 객체 생성이 연속으로 늘어남

    BE_LOG("\nNumber num10 = num2 - num3 - num4 - num5\n");
    Number num10 = num2 - num3 - num4 - num5 - num6; // ref-qualifier 에 의해 임시객체 생성이 최소화됨

    BE_LOG("\n");
}
