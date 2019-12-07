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

const RectF RectF::zero = RectF(0.0f, 0.0f, 0.0f, 0.0f);

RectF RectF::Add(const RectF &a) const {
    RectF rc;
    rc.x = Min(a.x, x);
    rc.y = Min(a.y, y);
    rc.w = Max(a.x + a.w, x + w) - rc.x;
    rc.h = Max(a.y + a.h, y + h) - rc.y;
    return rc;
}

RectF &RectF::Add(const RectF &a, const RectF &b) {
    x = Min(a.x, b.x);
    y = Min(a.y, b.y);
    w = Max(a.x + a.w, b.x + b.w) - x;
    h = Max(a.y + a.h, b.y + b.h) - y;
    return *this;
}

RectF &RectF::AddSelf(const RectF &a) {
    w = Max(a.x + a.w, x + w);
    h = Max(a.y + a.h, y + h);
    x = Min(a.x, x);
    y = Min(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

RectF RectF::AddPoint(const PointF &a) const {
    RectF rc;
    rc.x = Min(a.x, x);
    rc.y = Min(a.y, y);
    rc.w = Max(a.x, x + w) - rc.x;
    rc.h = Max(a.y, y + h) - rc.y;
    return rc;
}

RectF &RectF::AddPoint(const PointF &a, const PointF &b) {
    x = Min(a.x, b.x);
    y = Min(a.y, b.y);
    w = Max(a.x, b.x) - x;
    h = Max(a.y, b.y) - y;
    return *this;
}

RectF &RectF::AddPointSelf(const PointF &a) {
    w = Max(a.x, x + w);
    h = Max(a.y, y + h);
    x = Min(a.x, x);
    y = Min(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

RectF RectF::Intersect(const RectF &a) const {
    RectF rc;
    rc.x = Max(a.x, x);
    rc.y = Max(a.y, y);
    rc.w = Min(a.x + a.w, x + w) - rc.x;
    rc.h = Min(a.y + a.h, y + h) - rc.y;
    return rc;
}

RectF &RectF::Intersect(const RectF &a, const RectF &b) {
    x = Max(a.x, b.x);
    y = Max(a.y, b.y);
    w = Min(a.x + a.w, b.x + b.w) - x;
    h = Min(a.y + a.h, b.y + b.h) - y;
    return *this;
}

RectF &RectF::IntersectSelf(const RectF &a) {
    w = Min(a.x + a.w, x + w);
    h = Min(a.y + a.h, y + h);
    x = Max(a.x, x);
    y = Max(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

RectF RectF::Move(float ax, float ay) const {
    RectF rect;
    rect.x = x + ax;
    rect.y = y + ay;
    return rect;
}

RectF &RectF::MoveSelf(float ax, float ay) {
    x += ax;
    y += ay;
    return *this;
}

RectF RectF::Shrink(float ax, float ay) const {
    RectF rect;
    rect.x = x + ax;
    rect.y = y + ay;
    rect.w = w - ax * 2;
    rect.h = h - ay * 2;
    return rect;
}

RectF &RectF::ShrinkSelf(float ax, float ay) {
    x += ax;
    y += ay;
    w -= ax * 2;
    h -= ay * 2;
    return *this;
}

RectF RectF::Expand(float ax, float ay) const {
    RectF rect;
    rect.x = x - ax;
    rect.y = y - ay;
    rect.w = w + ax * 2;
    rect.h = h + ay * 2;
    return rect;
}

RectF &RectF::ExpandSelf(float ax, float ay) {
    x -= ax;
    y -= ay;
    w += ax * 2;
    h += ay * 2;
    return *this;
}

RectF RectF::FromString(const char *str) {
    RectF r;
    sscanf(str, "%f %f %f %f", &r.x, &r.y, &r.w, &r.h);
    return r;
}

BE_NAMESPACE_END
