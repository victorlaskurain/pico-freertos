#include <variant>
#include <vla/hw_timer.hpp>
#include <vla/modbus_daemon.hpp>
#include <pico/stdlib.h>

namespace vla {

static const auto inter_frame_delay = vla::period_us{1750};
static const auto inter_char_delay  = vla::period_us{ 750};

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

using InputMsg = vla::serial_io::InputMsg;
using OutputMsg = vla::serial_io::OutputMsg;
using Buffer = vla::serial_io::Buffer;

struct TimeoutMsg {
    uint8_t ignore;
};
using DaemonMessage =
    std::variant<Buffer, TimeoutMsg, vla::rtu_message, int32_t>;
using DaemonQueue = vla::Queue<DaemonMessage>;

static bool must_transmit(const rtu_message &msg) {
    return msg.length > 0;
}

static alarm_id set_alarm(DaemonQueue &q, period_us us) {
    return set_alarm(
        us,
        [](alarm_id, void *data) -> int64_t {
            static_cast<DaemonQueue *>(data)->sendFromIsr(TimeoutMsg{});
            return 0;
        },
        &q);
}

void modbus_daemon(vla::serial_io::InputQueue::Sender inq,
                   vla::serial_io::OutputQueue::Sender outq,
                   RtuMessageHandler handle_indication) {
    DaemonQueue q{5};
    state_t state = state_t::INITIAL;
    uint8_t buffer[PDU_MAX];
    uint8_t buffer_i = 0;
    uint8_t chr;
    vla::rtu_message rtu_msg;
    alarm_id aid;

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    inq.send(InputMsg(Buffer::create(&chr, 1), q));
    auto get_char = [&inq, &q, &chr](const Buffer *b) -> uint8_t {
        auto res = b->data[0];
        inq.send(InputMsg(Buffer::create(&chr, 1), q));
        return res;
    };

    while (true) {
        // uncomment to debug state transitions
        // static auto last_state = state_t::UNKNOWN;
        // if (last_state != state) {
        //     printf("%s\n", state_name(state));
        //     last_state = state;
        // }
        if (state_t::INITIAL == state) {
            aid = set_alarm(q, inter_frame_delay);
            auto msg = q.receive();
            // ignore chars in this state
            if (auto input_msg = std::get_if<Buffer>(&msg)) {
                get_char(input_msg);
                cancel_alarm(aid);
                // set_alarm(q, inter_frame_delay);
            } else if (std::get_if<TimeoutMsg>(&msg)) {
                state = state_t::READY;
            } else {
                state = state_t::INITIAL;
            }
            continue;
        }
        if (state_t::READY == state) {
            auto msg = q.receive();
            if (auto input_msg = std::get_if<Buffer>(&msg)) {
                chr = get_char(input_msg);
                set_alarm(q, inter_frame_delay);
                buffer[buffer_i++] = chr;
                state = state_t::RECEPTION;
            } else if (auto rtu_msg_ptr = std::get_if<vla::rtu_message>(&msg)) {
                if (must_transmit(*rtu_msg_ptr)) {
                    rtu_msg = *rtu_msg_ptr;
                    state = state_t::EMISSION;
                }
            } else {
                state = state_t::INITIAL;
            }
            continue;
        }
        if (state_t::RECEPTION == state) {
            auto msg = q.receive();
            if (auto input_msg = std::get_if<Buffer>(&msg)) {
                chr = get_char(input_msg);
                cancel_alarm(aid);
                // discard the timeout message that might have
                // been enqueued during pop and cancellation
                DaemonMessage discard;
                while (q.peek(discard, 0) &&
                       std::get_if<TimeoutMsg>(&discard)) {
                    q.receive();
                }
                set_alarm(q, inter_frame_delay);
                buffer[buffer_i++] = chr;
            } else if (std::get_if<TimeoutMsg>(&msg)) {
                rtu_msg = rtu_message(buffer, buffer_i);
                buffer_i = 0;
                state = state_t::PROCESSING;
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
            // printf("handle_indication begin %d\n", (int)rtu_msg.length);
            handle_indication(rtu_msg, rtu_msg);
            // printf("handle_indication done %d\n", (int)rtu_msg.length);
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
            outq.send(OutputMsg(Buffer::create(rtu_msg.buffer, rtu_msg.length), q));
            q.receive();
            state = state_t::INITIAL;
            continue;
        }
    }
}

} // namespace vla
