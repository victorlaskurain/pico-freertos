#include <pico/time.h>
#include <vla/hw_timer.hpp>

namespace vla {

AlarmId set_alarm(PeriodUs us, AlarmCb cb, void *data) {
    auto id = add_alarm_in_us(us.us, (alarm_callback_t)cb, data, true);
    return AlarmId(id);
}

void cancel_alarm(AlarmId id) {
    ::cancel_alarm(id.id);
}

} // namespace vla
