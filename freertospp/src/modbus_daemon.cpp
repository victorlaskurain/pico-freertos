#include <pico/stdlib.h>
#include <vla/hw_timer.hpp>
#include <vla/modbus_daemon.hpp>

namespace vla {

#ifdef MODBUS_RTU_STD_TIMEOUTS
static const auto inter_frame_delay = PeriodUs{1750};
static const auto inter_char_delay  = PeriodUs{750};
#else
static const auto inter_frame_delay = PeriodUs{75};
static const auto inter_char_delay  = PeriodUs{15};
#endif

using vla::serial_io::BytesWritten;

enum class state_t : uint8_t {
    INITIAL,
    READY,
    EMISSION,
    RECEPTION,
    PROCESSING,
    UNKNOWN
};

const char *state_name(state_t s) {
    if (state_t::INITIAL == s)
        return "INITIAL";
    if (state_t::READY == s)
        return "READY";
    if (state_t::EMISSION == s)
        return "EMISSION";
    if (state_t::RECEPTION == s)
        return "RECEPTION";
    if (state_t::PROCESSING == s)
        return "PROCESSING";
    return "XXX";
};

static AlarmId set_alarm(ModbusDaemonQueue &q, PeriodUs us) {
    return set_alarm(
        us,
        [](AlarmId, void *data) -> int64_t {
            static_cast<ModbusDaemonQueue *>(data)->sendFromIsr(TimeoutMsg{});
            return 0;
        },
        &q);
}

using InputMsg  = vla::serial_io::InputMsg;
using OutputMsg = vla::serial_io::OutputMsg;
using Buffer    = vla::serial_io::Buffer;

static bool must_transmit(const RtuMessage &msg) {
    return msg.length > 0;
}

void modbus_daemon(ModbusDaemonQueue &q,
                   vla::serial_io::OutputQueue::Sender outq,
                   RtuMessageHandler handle_indication) {
    state_t state = state_t::INITIAL;
    uint8_t buffer[PDU_MAX];
    uint8_t buffer_i = 0;
    uint8_t chr;
    vla::RtuMessage rtu_msg;
    AlarmId aid;

    while (true) {
        // uncomment to debug state transitions
        // static auto last_state = state_t::UNKNOWN;
        // if (last_state != state) {
        //     printf("\n\n%s\n", state_name(state));
        //     printf("==========\n");
        //     write(1, buffer, buffer_i);
        //     printf("==========\n");
        //     last_state = state;
        // }
        if (state_t::INITIAL == state) {
            aid      = set_alarm(q, inter_frame_delay);
            auto msg = q.receive();
            // ignore chars in this state
            if (std::get_if<ReadChar>(&msg)) {
                cancel_alarm(aid);
            } else if (std::get_if<TimeoutMsg>(&msg)) {
                state = state_t::READY;
            }
            continue;
        }
        if (state_t::READY == state) {
            auto msg = q.receive();
            if (auto input_msg = std::get_if<ReadChar>(&msg)) {
                chr                = input_msg->chr;
                aid                = set_alarm(q, inter_frame_delay);
                buffer[buffer_i++] = chr;
                state              = state_t::RECEPTION;
            } else if (auto rtu_msg_ptr = std::get_if<vla::RtuMessage>(&msg)) {
                if (must_transmit(*rtu_msg_ptr)) {
                    rtu_msg = *rtu_msg_ptr;
                    state   = state_t::EMISSION;
                }
            } else {
                state = state_t::INITIAL;
            }
            continue;
        }
        if (state_t::RECEPTION == state) {
            auto msg = q.receive();
            if (auto input_msg = std::get_if<ReadChar>(&msg)) {
                chr = input_msg->chr;
                cancel_alarm(aid);
                // discard the timeout message that might have
                // been enqueued during pop and cancellation
                ModbusDaemonMessage discard;
                while (q.peek(discard, 0) &&
                       std::get_if<TimeoutMsg>(&discard)) {
                    q.receive();
                }
                buffer[buffer_i++] = chr;
                aid                = set_alarm(q, inter_frame_delay);
            } else if (std::get_if<TimeoutMsg>(&msg)) {
                rtu_msg  = RtuMessage(buffer, buffer_i);
                buffer_i = 0;
                state    = state_t::PROCESSING;
            } else {
                state = state_t::INITIAL;
            }
            continue;
        }
        if (state_t::PROCESSING == state) {
            // we set the timeout for the inter frame delay and
            // process the message. After processing we wait for the
            // timeout. Anything else but the timeout is an error.
            set_alarm(q, inter_frame_delay - inter_char_delay);
            handle_indication(rtu_msg, rtu_msg);
            auto msg = q.receive();
            if (std::get_if<TimeoutMsg>(&msg)) {
                q.send(rtu_msg);
                state = state_t::READY;
            } else {
                state = state_t::INITIAL;
            }
            continue;
        }
        if (state_t::EMISSION == state) {
            outq.send(
                OutputMsg(Buffer::create(rtu_msg.buffer, rtu_msg.length), q));
            for (auto msg = q.receive(); !std::get_if<BytesWritten>(&msg);
                 msg      = q.receive()) {
                // do nothing, just wait for the message
            }
            state = state_t::INITIAL;
            continue;
        }
    }
}

} // namespace vla
