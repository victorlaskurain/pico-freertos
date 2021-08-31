#ifndef VLA_HW_TIMER_HPP
#define VLA_HW_TIMER_HPP

#include <cstdint>

namespace vla {

struct period_us {
    uint32_t us;
    constexpr explicit period_us(uint32_t us) : us(us) {
    }
    period_us operator-(period_us other) const {
        return period_us{us - other.us};
    }
};

struct timestamp_us {
    uint32_t us = 0;
    explicit timestamp_us(uint32_t us) : us(us) {
    }
    timestamp_us(const timestamp_us &other) : us(other.us) {
    }
    timestamp_us(const volatile timestamp_us &other) : us(other.us) {
    }
    timestamp_us &operator+=(period_us delta) {
        us += delta.us;
        return *this;
    }
    volatile timestamp_us &operator+=(period_us delta) volatile {
        us += delta.us;
        return *this;
    }
    timestamp_us operator+(period_us delta) volatile {
        auto newus = *this;
        return newus += delta;
    }
    timestamp_us operator+(period_us delta) {
        auto newus = *this;
        return newus += delta;
    }
    bool operator<(timestamp_us other) const {
        return us < other.us;
    }
};
inline period_us operator-(timestamp_us a, timestamp_us b) {
    if (a < b) {
        return period_us(0);
    }
    return period_us(a.us - b.us);
}

struct alarm_id {
    int32_t id = 0;
    explicit alarm_id(int32_t id = 0) : id(id) {
    }
    explicit operator bool() const {
        return id > 0;
    }
    bool operator==(alarm_id other) const {
        return id == other.id;
    }
    bool operator!=(alarm_id other) const {
        return !(*this == other);
    }
};

using alarm_cb_t = int64_t (*)(alarm_id, void *);

alarm_id set_alarm(period_us us, alarm_cb_t, void *data);
void cancel_alarm(alarm_id id);

} // namespace vla

#endif // VLA_HW_TIMER
