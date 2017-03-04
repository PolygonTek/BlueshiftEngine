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

#include "Core/Str.h"
#include "Timespan.h"

BE_NAMESPACE_BEGIN

class BE_API DateTime {
public:
    // Days of the week in 7-day calendars
    enum DayOfWeek {
        Monday = 0,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    };

    // Months of the year in 12-month calendars.
    enum MonthOfYear {
        January = 1,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };

    struct Date {
        int32_t     day;
        int32_t     month;
        int32_t     year;
    };
    
    struct Time {
        int32_t     hour;
        int32_t     minute;
        int32_t     second;
        int32_t     millisecond;
    };

    DateTime() {}
    DateTime(int64_t ticks) { this->ticks = ticks; }
    DateTime(int32_t year, int32_t month, int32_t day, int32_t hour = 0, int32_t min = 0, int32_t sec = 0, int32_t msec = 0);
    
    DateTime            operator+(const Timespan &rhs) const;
    DateTime            operator-(const Timespan &rhs) const;
    Timespan            operator-(const DateTime &rhs) const;
    DateTime            operator+=(const Timespan &rhs);
    DateTime            operator-=(const Timespan &rhs);
    
    bool                operator==(const DateTime &rhs) const;
    bool                operator!=(const DateTime &rhs) const;
    bool                operator>(const DateTime &rhs) const;
    bool                operator>=(const DateTime &rhs) const;
    bool                operator<(const DateTime &rhs) const;
    bool                operator<=(const DateTime &rhs) const;
    
    int32_t             Year() const { return ToDate().year; }
    int32_t             Month() const { return ToDate().month; }
    MonthOfYear         GetMonthOfYear() const { return static_cast<MonthOfYear>(Month()); }
    int32_t             Day() const { return ToDate().day; }
    double              JulianDay() const { return (double)(1721425.5 + ticks / Timespan::TicksPerDay); }
    double              ModifiedJulianDay() const { return (JulianDay() - 2400000.5); }
    enum DayOfWeek      DayOfWeek() const { return static_cast<enum DayOfWeek>((ticks / Timespan::TicksPerDay) % 7); }
    int32_t             DayOfYear() const;
    int32_t             Hour() const { return (int32_t)((ticks / Timespan::TicksPerHour) % 24); }
    int32_t             Hour12() const;
    bool                IsAM() const { return Hour() < 12; }
    bool                IsPM() const { return Hour() >= 12; }
    int32_t             Minute() const { return (int32_t)((ticks / Timespan::TicksPerMinute) % 60); }
    int32_t             Second() const { return (int32_t)((ticks / Timespan::TicksPerSecond) % 60); }
    int32_t             Millisecond() const { return (int32_t)((ticks / Timespan::TicksPerMillisecond) % 1000); }
    int64_t             Ticks() const { return ticks; }
    
    DateTime            DatePart() const { return DateTime(ticks - (ticks % Timespan::TicksPerDay)); }
    
    Date                ToDate() const;
    Time                ToTime() const;
    Str                 ToString() const;
    
    static DateTime     Now();
    static DateTime     UtcNow();
    static DateTime     Today() { return Now().DatePart(); }
    
                        // The minimum date value is January 1, 0001, 00:00:00.0.
    static DateTime     MinValue() { return DateTime(0); }
                        // The maximum date value is December 31, 9999, 23:59:59.9999999.
    static DateTime     MaxValue() { return DateTime(3652059 * Timespan::TicksPerDay - 1); }
    
    static int32_t      DaysInMonth(int32_t year, int32_t month);
    static int32_t      DaysInYear(int32_t year);
    static bool         IsLeapYear(int32_t year);

private:
    int64_t             ticks;
};

BE_INLINE DateTime DateTime::operator+(const Timespan &rhs) const {
    return DateTime(ticks + rhs.Ticks());
}

BE_INLINE DateTime DateTime::operator-(const Timespan &rhs) const {
    return DateTime(ticks - rhs.Ticks());
}

BE_INLINE Timespan DateTime::operator-(const DateTime &rhs) const {
    return Timespan(ticks - rhs.ticks);
}

BE_INLINE DateTime DateTime::operator+=(const Timespan &rhs) {
    ticks += rhs.Ticks();
    return *this;
}

BE_INLINE DateTime DateTime::operator-=(const Timespan &rhs) {
    ticks -= rhs.Ticks();
    return *this;
}

BE_INLINE bool DateTime::operator==(const DateTime &rhs) const {
    return ticks == rhs.ticks;
}

BE_INLINE bool DateTime::operator!=(const DateTime &rhs) const {
    return ticks != rhs.ticks;
}

BE_INLINE bool DateTime::operator>(const DateTime &rhs) const {
    return ticks > rhs.ticks;
}

BE_INLINE bool DateTime::operator>=(const DateTime &rhs) const {
    return ticks >= rhs.ticks;
}

BE_INLINE bool DateTime::operator<(const DateTime &rhs) const {
    return ticks < rhs.ticks;
}

BE_INLINE bool DateTime::operator<=(const DateTime &rhs) const {
    return ticks <= rhs.ticks;
}

BE_NAMESPACE_END
