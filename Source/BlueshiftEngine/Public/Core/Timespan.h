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

BE_NAMESPACE_BEGIN

class BE_API Timespan {
public:
    /// The number of Timespan ticks per week.
    static const int64_t TicksPerWeek = 6048000000000;
    
    /// The number of Timespan ticks per day.
    static const int64_t TicksPerDay = 864000000000;
    
    /// The number of Timespan ticks per hour.
    static const int64_t TicksPerHour = 36000000000;
    
    /// The number of Timespan ticks per minute.
    static const int64_t TicksPerMinute = 600000000;
    
    /// The number of Timespan ticks per second.
    static const int64_t TicksPerSecond = 10000000;
    
    /// The number of Timespan ticks per millisecond.
    static const int64_t TicksPerMillisecond = 10000;
    
    Timespan() {}
    Timespan(int64_t ticks) { this->ticks = ticks; }
    Timespan(int32_t hours, int32_t minutes, int32_t seconds) { Assign(0, hours, minutes, seconds, 0); }
    Timespan(int32_t days, int32_t hours, int32_t minutes, int32_t seconds) { Assign(days, hours, minutes, seconds, 0); }
    Timespan(int32_t days, int32_t hours, int32_t minutes, int32_t seconds, int32_t milliseconds) { Assign(days, hours, minutes, seconds, milliseconds); }
    
    void            Assign(int32_t days, int32_t hours, int32_t minutes, int32_t seconds, int32_t milliseconds);
    
    Timespan        operator-() const;
    
    Timespan        operator+(const Timespan &rhs) const;
    Timespan        operator-(const Timespan &rhs) const;
    Timespan        operator*(const float rhs) const;
    friend Timespan operator*(float lhs, const Timespan &rhs);
    Timespan        operator+=(const Timespan &rhs);
    Timespan        operator-=(const Timespan &rhs);
    Timespan        operator*=(const float rhs);
    
    bool            operator==(const Timespan &rhs) const;
    bool            operator!=(const Timespan &rhs) const;
    bool            operator>(const Timespan &rhs) const;
    bool            operator>=(const Timespan &rhs) const;
    bool            operator<(const Timespan &rhs) const;
    bool            operator<=(const Timespan &rhs) const;
    
    Timespan        Duration() const { return Timespan(ticks >= 0 ? ticks : -ticks); }
    
    int32_t         Days() const { return (int32_t)(ticks / TicksPerDay); }
    int32_t         Hours() const { return (int32_t)((ticks / TicksPerHour) % 24); }
    int32_t         Minuts() const { return (int32_t)((ticks / TicksPerMinute) % 60); }
    int32_t         Seconds() const { return (int32_t)((ticks / TicksPerSecond) % 1000); }
    int32_t         Milliseconds() const { return (int32_t)((ticks / TicksPerMillisecond) % 1000); }
    
    int64_t         Ticks() const { return ticks; }
    
    double          TotalDays() const { return ((float)ticks / TicksPerDay); }
    double          TotalHours() const { return ((float)ticks / TicksPerHour); }
    double          TotalMinutes() const { return ((float)ticks / TicksPerMinute); }
    double          TotalSeconds() const { return ((float)ticks / TicksPerSecond); }
    double          TotalMilliseconds() const { return ((float)ticks / TicksPerMillisecond); }
    
    static Timespan FromDays(double days);
    static Timespan FromHours(double hours);
    static Timespan FromMinutes(double minutes);
    static Timespan FromSeconds(double seconds);
    static Timespan FromMilliseconds(double milliseconds);
        
                    /// The minimum time span value is slightly less than -10,675,199 days.
    static Timespan MinValue() { return Timespan(-9223372036854775807 - 1); }
                    /// The maximum time span value is slightly more than 10,675,199 days.
    static Timespan MaxValue() { return Timespan(9223372036854775807); }
    
private:
    int64_t         ticks;
};

BE_INLINE Timespan Timespan::operator-() const {
    return Timespan(-ticks);
}

BE_INLINE Timespan Timespan::operator+(const Timespan &rhs) const {
    return Timespan(ticks + rhs.ticks);
}

BE_INLINE Timespan Timespan::operator-(const Timespan &rhs) const {
    return Timespan(ticks - rhs.ticks);
}

BE_INLINE Timespan Timespan::operator*(const float rhs) const {
    return Timespan((int64_t)(ticks * rhs));
}

BE_INLINE Timespan operator*(float lhs, const Timespan &rhs) {
    return rhs.operator*(lhs);
}

BE_INLINE Timespan Timespan::operator+=(const Timespan &rhs) {
    ticks += rhs.ticks;
    return *this;
}

BE_INLINE Timespan Timespan::operator-=(const Timespan &rhs) {
    ticks -= rhs.ticks;
    return *this;
}

BE_INLINE Timespan Timespan::operator*=(const float rhs) {
    ticks = (int64_t)(ticks * rhs);
    return *this;
}

BE_INLINE bool Timespan::operator==(const Timespan &rhs) const {
    return ticks == rhs.ticks;
}

BE_INLINE bool Timespan::operator!=(const Timespan &rhs) const {
    return ticks != rhs.ticks;
}

BE_INLINE bool Timespan::operator>(const Timespan &rhs) const {
    return ticks > rhs.ticks;
}

BE_INLINE bool Timespan::operator>=(const Timespan &rhs) const {
    return ticks >= rhs.ticks;
}

BE_INLINE bool Timespan::operator<(const Timespan &rhs) const {
    return ticks < rhs.ticks;
}

BE_INLINE bool Timespan::operator<=(const Timespan &rhs) const {
    return ticks <= rhs.ticks;
}

BE_NAMESPACE_END
