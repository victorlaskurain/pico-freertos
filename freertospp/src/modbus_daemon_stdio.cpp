#include <pico/stdlib.h>
#include <unistd.h>
#include <vla/hw_timer.hpp>
#include <vla/modbus_daemon.hpp>

namespace vla {

void get_chars_stdin_timer(ModbusDaemonQueue::SenderIsr *q) {
    static AlarmId id;
    if (id) {
        return;
    }
    auto handler = [](AlarmId, void *d) -> int64_t {
        BaseType_t taskWoken;
        auto q   = static_cast<ModbusDaemonQueue::SenderIsr *>(d);
        auto chr = getchar_timeout_us(0);
        while (chr >= 0) {
            q->send(ReadChar(time_us_64(), chr), &taskWoken);
            chr = getchar_timeout_us(0);
        }
        return -500;
    };
    vla::set_alarm(PeriodUs(500), handler, q);
}

void modbus_daemon_stdin(vla::serial_io::OutputQueue::Sender outq,
                         RtuMessageHandler handle_indication) {
    ModbusDaemonQueue q{32};
    auto sender_isr = q.sender_isr();
    get_chars_stdin_timer(&sender_isr);
    modbus_daemon(q, outq, handle_indication);
}

} // namespace vla
