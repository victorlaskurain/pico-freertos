#ifndef VLA_HW_TIMER_HPP
#define VLA_HW_TIMER_HPP

#include <cstdint>

namespace vla {

struct PeriodUs {
    uint32_t us;
    constexpr explicit PeriodUs(uint32_t us) : us(us) {
    }
    PeriodUs operator-(PeriodUs other) const {
        return PeriodUs{us - other.us};
    }
};

struct TimestampUs {
    uint32_t us = 0;
    explicit TimestampUs(uint32_t us) : us(us) {
    }
    TimestampUs(const TimestampUs &other) : us(other.us) {
    }
    TimestampUs(const volatile TimestampUs &other) : us(other.us) {
    }
    TimestampUs &operator+=(PeriodUs delta) {
        us += delta.us;
        return *this;
    }
    volatile TimestampUs &operator+=(PeriodUs delta) volatile {
        us += delta.us;
        return *this;
    }
    TimestampUs operator+(PeriodUs delta) volatile {
        auto newus = *this;
        return newus += delta;
    }
    TimestampUs operator+(PeriodUs delta) {
        auto newus = *this;
        return newus += delta;
    }
    bool operator<(TimestampUs other) const {
        return us < other.us;
    }
};
inline PeriodUs operator-(TimestampUs a, TimestampUs b) {
    if (a < b) {
        return PeriodUs(0);
    }
    return PeriodUs(a.us - b.us);
}

struct AlarmId {
    int32_t id = 0;
    explicit AlarmId(int32_t id = 0) : id(id) {
    }
    explicit operator bool() const {
        return id > 0;
    }
    bool operator==(AlarmId other) const {
        return id == other.id;
    }
    bool operator!=(AlarmId other) const {
        return !(*this == other);
    }
};

using AlarmCb = int64_t (*)(AlarmId, void *);

AlarmId set_alarm(PeriodUs us, AlarmCb, void *data);
void cancel_alarm(AlarmId id);

} // namespace vla

#endif // VLA_HW_TIMER
