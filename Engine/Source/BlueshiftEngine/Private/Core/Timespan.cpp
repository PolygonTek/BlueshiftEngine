#include "Precompiled.h"
#include "Core/Timespan.h"

BE_NAMESPACE_BEGIN

void Timespan::Assign(int32_t days, int32_t hours, int32_t minutes, int32_t seconds, int32_t milliseconds) {
    int64_t totalms = 1000 * (60 * 60 * 24 * (int64_t)days +
                              60 * 60 * (int64_t)hours +
                              60 * (int64_t)minutes + (int64_t)seconds) + (int64_t)milliseconds;
    
    assert((totalms >= MinValue().TotalMilliseconds()) && (totalms <= MaxValue().TotalMilliseconds()));
    
    ticks = totalms * Timespan::TicksPerMillisecond;
}

Timespan Timespan::FromDays(double days) {
    assert((days >= MinValue().TotalDays()) && (days <= MaxValue().TotalDays()));
    
    return Timespan(days * Timespan::TicksPerDay);
}

Timespan Timespan::FromHours(double hours) {
    assert((hours >= MinValue().TotalHours()) && (hours <= MaxValue().TotalHours()));
    
    return Timespan(hours * Timespan::TicksPerHour);
}

Timespan Timespan::FromMinutes(double minutes) {
    assert((minutes >= MinValue().TotalMinutes()) && (minutes <= MaxValue().TotalMinutes()));
    
    return Timespan(minutes * Timespan::TicksPerMinute);
}

Timespan Timespan::FromSeconds(double seconds) {
    assert((seconds >= MinValue().TotalMinutes()) && (seconds <= MaxValue().TotalMinutes()));
    
    return Timespan(seconds * Timespan::TicksPerSecond);
}

Timespan Timespan::FromMilliseconds(double milliseconds) {
    assert((milliseconds >= MinValue().TotalMilliseconds()) && (milliseconds <= MaxValue().TotalMilliseconds()));
    
    return Timespan(milliseconds * Timespan::TicksPerMillisecond);
}

BE_NAMESPACE_END
