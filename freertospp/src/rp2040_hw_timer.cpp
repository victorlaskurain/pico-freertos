#include <vla/hw_timer.hpp>
#include <pico/time.h>

namespace vla {

alarm_id set_alarm(period_us us, alarm_cb_t cb, void *data) {
    auto id = add_alarm_in_us(us.us, (alarm_callback_t)cb, data, true);
    return alarm_id(id);
}

void cancel_alarm(alarm_id id){
    ::cancel_alarm(id.id);
}

} // namespace vla
