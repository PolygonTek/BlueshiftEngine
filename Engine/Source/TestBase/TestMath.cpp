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
#include "TestMath.h"

void TestMath() {
    BE1::Mat3 m;
    m.SetIdentity();
    m.RotateX(-30);
    m.RotateY(80);
    m.RotateZ(15);

    BE1::Rotation r = m.ToRotation();
    BE1::Mat3 m2 = r.ToMat3();
    assert(m.Equals(m2, 0.001f));

    BE1::Quat q = m.ToQuat();
    BE1::Mat3 m3 = q.ToMat3();
    assert(m.Equals(m3, 0.01f));

    BE1::Angles a = m.ToAngles();
    BE1::Mat3 m4 = a.ToMat3();
    assert(m.Equals(m4, 0.01f));

    BE1::Rotation r2 = q.ToRotation();
    BE1::Mat3 m5 = r2.ToMat3();
    assert(m.Equals(m5, 0.01f));

    BE1::Angles a2 = q.ToAngles();
    BE1::Mat3 m6 = a2.ToMat3();
    assert(m.Equals(m6, 0.01f));
    assert(a.Equals(a2, 0.01f));

    BE1::Quat q2 = a.ToQuat();
    BE1::Mat3 m7 = q2.ToMat3();
    assert(m.Equals(m7, 0.01f));
    assert(q.Equals(q2, 0.01f));

    BE1::Angles a3 = r.ToAngles();
    BE1::Mat3 m8 = a3.ToMat3();
    assert(m.Equals(m8, 0.01f));
    assert(a.Equals(a3, 0.01f));

    BE1::Quat q3 = r.ToQuat();
    BE1::Mat3 m9 = q3.ToMat3();
    assert(m.Equals(m9, 0.01f));
    assert(q.Equals(q3, 0.01f));
}
