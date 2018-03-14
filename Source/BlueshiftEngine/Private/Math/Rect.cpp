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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Rect Rect::empty = Rect(0, 0, 0, 0);

Rect Rect::Add(const Rect &a) const {
    Rect rc;
    rc.x = Min(a.x, x);
    rc.y = Min(a.y, y);
    rc.w = Max(a.x + a.w, x + w) - rc.x;
    rc.h = Max(a.y + a.h, y + h) - rc.y;
    return rc;
}

Rect &Rect::Add(const Rect &a, const Rect &b) {
    x = Min(a.x, b.x);
    y = Min(a.y, b.y);
    w = Max(a.x + a.w, b.x + b.w) - x;
    h = Max(a.y + a.h, b.y + b.h) - y;
    return *this;
}

Rect &Rect::AddSelf(const Rect &a) {
    w = Max(a.x + a.w, x + w);
    h = Max(a.y + a.h, y + h);
    x = Min(a.x, x);
    y = Min(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

Rect Rect::AddPoint(const Point &a) const {
    Rect rc;
    rc.x = Min(a.x, x);
    rc.y = Min(a.y, y);
    rc.w = Max(a.x, x + w) - rc.x;
    rc.h = Max(a.y, y + h) - rc.y;
    return rc;
}

Rect &Rect::AddPoint(const Point &a, const Point &b) {
    x = Min(a.x, b.x);
    y = Min(a.y, b.y);
    w = Max(a.x, b.x) - x;
    h = Max(a.y, b.y) - y;
    return *this;
}

Rect &Rect::AddPointSelf(const Point &a) {
    w = Max(a.x, x + w);
    h = Max(a.y, y + h);
    x = Min(a.x, x);
    y = Min(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

Rect Rect::Intersect(const Rect &a) const {
    Rect rc;
    rc.x = Max(a.x, x);
    rc.y = Max(a.y, y);
    rc.w = Min(a.x + a.w, x + w) - rc.x;
    rc.h = Min(a.y + a.h, y + h) - rc.y;
    return rc;
}

Rect &Rect::Intersect(const Rect &a, const Rect &b) {
    x = Max(a.x, b.x);
    y = Max(a.y, b.y);
    w = Min(a.x + a.w, b.x + b.w) - x;
    h = Min(a.y + a.h, b.y + b.h) - y;
    return *this;
}

Rect &Rect::IntersectSelf(const Rect &a) {
    w = Min(a.x + a.w, x + w);
    h = Min(a.y + a.h, y + h);
    x = Max(a.x, x);
    y = Max(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

Rect Rect::Move(int ax, int ay) const {
    Rect rect;
    rect.x = x + ax;
    rect.y = y + ay;
    return rect;
}

Rect &Rect::MoveSelf(int ax, int ay) {
    x += ax;
    y += ay;
    return *this;
}

Rect Rect::Shrink(int ax, int ay) const {
    Rect rect;
    rect.x = x + ax;
    rect.y = y + ay;
    rect.w = w - ax * 2;
    rect.h = h - ay * 2;
    return rect;
}

Rect &Rect::ShrinkSelf(int ax, int ay) {
    x += ax;
    y += ay;
    w -= ax * 2;
    h -= ay * 2;
    return *this;
}

Rect Rect::Expand(int ax, int ay) const {
    Rect rect;
    rect.x = x - ax;
    rect.y = y - ay;
    rect.w = w + ax * 2;
    rect.h = h + ay * 2;
    return rect;
}

Rect &Rect::ExpandSelf(int ax, int ay) {
    x -= ax;
    y -= ay;
    w += ax * 2;
    h += ay * 2;
    return *this;
}

Rect Rect::FromString(const char *str) {
    Rect r;
    sscanf(str, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
    return r;
}

BE_NAMESPACE_END
