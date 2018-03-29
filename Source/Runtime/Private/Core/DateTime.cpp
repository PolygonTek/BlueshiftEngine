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
#include "Core/Str.h"
#include "Math/Math.h"
#include "Platform/PlatformTime.h"
#include "Core/DateTime.h"

BE_NAMESPACE_BEGIN

// Holds the days per month in a non-leap year.
static const int32_t daysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
// Holds the cumulative days per month in a non-leap year.
static const int32_t daysToMonth[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

DateTime::DateTime(int32_t year, int32_t month, int32_t day, int32_t hour, int32_t min, int32_t sec, int32_t msec) {
    assert((year >= 1) && (year <= 9999));
    assert((month >= 1) && (month <= 12));
    assert((day >= 1) && (day <= DaysInMonth(year, month)));
    assert((hour >= 0) && (hour <= 23));
    assert((min >= 0) && (min <= 59));
    assert((sec >= 0) && (sec <= 59));
    assert((msec >= 0) && (msec <= 999));
    
    int32_t totalDays = 0;
    
    if (month > 2 && IsLeapYear(year)) {
        totalDays++;
    }
    
    year--;     // the current year is not a full year yet
    month--;    // the current month is not a full month yet
    
    totalDays += year * 365;
    totalDays += year / 4;              // leap year day every four years...
    totalDays -= year / 100;            // ...except every 100 years...
    totalDays += year / 400;            // ...but also every 400 years
    totalDays += daysToMonth[month];    // days in this year up to last month
    totalDays += day - 1;               // days in this month minus today
    
    ticks = totalDays * Timespan::TicksPerDay
        + hour * Timespan::TicksPerHour
        + min * Timespan::TicksPerMinute
        + sec * Timespan::TicksPerSecond
        + msec * Timespan::TicksPerMillisecond;
}

DateTime::Date DateTime::ToDate() const {
    Date date;
    
    // Based on FORTRAN code in:
    // Fliegel, H. F. and van Flandern, T. C.,
    // Communications of the ACM, Vol. 11, No. 10 (October 1968).
    
    int32_t i, j, k, l, n;
    
    l = Math::Floor(JulianDay() + 0.5) + 68569;
    n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    j = 80 * l / 2447;
    k = l - 2447 * j / 80;
    l = j / 11;
    j = j + 2 - 12 * l;
    i = 100 * (n - 49) + i + l;
    
    date.year = i;
    date.month = j;
    date.day = k;
    
    return date;
}

DateTime::Time DateTime::ToTime() const {
    Time time;
    time.hour = Hour();
    time.minute = Minute();
    time.second = Second();
    time.millisecond = Millisecond();
    
    return time;
}

Str DateTime::ToString() const {
    Str str;
    str.sPrintf("%04d.%02d.%02d-%02d.%02d.%02d", Year(), Month(), Day(), Hour(), Minute(), Second());
    return str;
}

int32_t DateTime::DayOfYear() const {
    Date date = ToDate();
    int32_t day = date.day;
    
    for (int32_t month = 1; month < date.month; month++) {
        day += DaysInMonth(date.year, month);
    }
    
    return day;
}

int32_t DateTime::Hour12() const {
    int32_t hour = Hour();
    if (hour == 0) {
        hour = 12;
    } else if (hour > 12) {
        hour -= 12;
    }
    return hour;
}

DateTime DateTime::Now() {
    Date date;
    int32_t dayOfWeek;
    Time time;
    
    PlatformTime::LocalTime(date.year, date.month, dayOfWeek, date.day, time.hour, time.minute, time.second, time.millisecond);
    
    return DateTime(date.year, date.month, date.day, time.hour, time.minute, time.second, time.millisecond);
}

DateTime DateTime::UtcNow() {
    Date date;
    int32_t dayOfWeek;
    Time time;
    
    PlatformTime::UtcTime(date.year, date.month, dayOfWeek, date.day, time.hour, time.minute, time.second, time.millisecond);
    
    return DateTime(date.year, date.month, date.day, time.hour, time.minute, time.second, time.millisecond);
}

int32_t DateTime::DaysInMonth(int32_t year, int32_t month) {
    assert((month >= 1) && (month <= 12));
    
    if ((month == MonthOfYear::February) && IsLeapYear(year)) {
        return 29;
    }
    
    return daysPerMonth[month];
}

int32_t DateTime::DaysInYear(int32_t year) {
    if (IsLeapYear(year)) {
        return 366;
    }
    
    return 365;
}

bool DateTime::IsLeapYear(int32_t year) {
    if ((year % 4) == 0) {
        return (((year % 100) != 0) || ((year % 400) == 0));
    }
    
    return false;
}

BE_NAMESPACE_END
