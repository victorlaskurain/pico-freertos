#include <mp/fsm.h>
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
        [](AlarmId aid, void *data) -> int64_t {
            static_cast<ModbusDaemonQueue *>(data)->sendFromIsr(
                TimeoutMsg{aid});
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

// events:
// ReadChar, TimeoutMsg, vla::RtuMessage, vla::serial_io::BytesWritten
struct MdeStart {};
struct MdsStart {};
struct MdsInitial {
    AlarmId aid;
    MdsInitial(AlarmId aid) : aid(aid) {
    }
};
struct MdsReady {};
struct MdsEmission {};
struct MdsReception {
    AlarmId aid;
    uint8_t *buffer;
    uint8_t buffer_i = 0;
    MdsReception(AlarmId aid, uint8_t *buffer) : aid(aid), buffer(buffer) {
    }
    void append_char(uint8_t chr) {
        buffer[buffer_i++] = chr;
    }
};
struct MdsProcessing {
    RtuMessage rtu_msg;
    AlarmId aid;
    MdsProcessing(RtuMessage m, AlarmId a) : rtu_msg(m), aid(a) {
    }
};

using ModbusDaemonState =
    std::variant<MdsStart, MdsInitial, MdsReady, MdsReception, MdsProcessing,
                 MdsEmission>;
class ModbusDaemonFsm : public mp::fsm<ModbusDaemonFsm, ModbusDaemonState> {
    ModbusDaemonQueue &q;
    vla::serial_io::OutputQueue::Sender outq;
    RtuMessageHandler handle_indication;
    uint8_t buffer[PDU_MAX];
    uint8_t buffer_i = 0;

  public:
    ModbusDaemonFsm(ModbusDaemonQueue &q,
                    vla::serial_io::OutputQueue::Sender outq,
                    RtuMessageHandler h)
        : q(q), outq(outq), handle_indication(h) {
        this->dispatch(MdeStart());
    }

    template <typename State, typename Event>
    auto on_event(State &, const Event &) {
        // unexpected event
        return MdsInitial(set_alarm(q, inter_frame_delay));
    }

    /*
     * STATE MdsStart
     */
    auto on_event(MdsStart &, const MdeStart &evt) {
        return MdsInitial(set_alarm(q, inter_frame_delay));
    }

    /*
     * STATE MdsInitial
     */
    auto on_event(MdsInitial &state, const ReadChar &msg) {
        // ignore chars in this state and reset timer
        cancel_alarm(state.aid);
        state.aid = set_alarm(q, inter_frame_delay);
        return std::nullopt;
    }
    std::optional<ModbusDaemonState> on_event(MdsInitial &state,
                                              const TimeoutMsg tout) {
        if (state.aid != tout.aid) {
            return std::nullopt;
        }
        return MdsReady();
    }

    /*
     * STATE MdsReady
     */
    auto on_event(MdsReady &, const ReadChar input_msg) {
        auto new_state = MdsReception(set_alarm(q, inter_frame_delay), buffer);
        new_state.append_char(input_msg.chr);
        return new_state;
    }
    std::optional<ModbusDaemonState> on_event(MdsReady &,
                                              const vla::RtuMessage msg) {
        if (must_transmit(msg)) {
            outq.send(OutputMsg(Buffer::create(msg.buffer, msg.length), q));
            return MdsEmission();
        }
        return std::nullopt;
    }

    /*
     * STATE MdsReception
     */
    auto on_event(MdsReception &state, const ReadChar input_msg) {
        state.append_char(input_msg.chr);
        cancel_alarm(state.aid);
        state.aid = set_alarm(q, inter_frame_delay);
        return std::nullopt;
    }
    std::optional<ModbusDaemonState> on_event(MdsReception &state,
                                              const TimeoutMsg tout) {
        // we set the timeout for the inter frame delay and process
        // the message. After processing we wait for the timeout on
        // the processing state. Anything else but the timeout is an
        // error.
        if (tout.aid != state.aid) {
            return std::nullopt;
        }
        auto aid = set_alarm(q, inter_frame_delay - inter_char_delay);
        auto msg = RtuMessage(state.buffer, state.buffer_i);
        handle_indication(msg, msg);
        return MdsProcessing(msg, aid);
    }

    /*
     * STATE MdsProcessing
     */
    std::optional<ModbusDaemonState> on_event(MdsProcessing &state,
                                              const TimeoutMsg tout) {
        if (state.aid != tout.aid) {
            return std::nullopt;
        }
        q.send(state.rtu_msg);
        return MdsReady();
    }

    /*
     * STATE MdsEmission
     */
    auto on_event(MdsEmission &state, BytesWritten) {
        return MdsInitial(set_alarm(q, inter_frame_delay));
    }
    template <typename Event> auto on_event(MdsEmission &state, const Event &) {
        return std::nullopt;
    }
};

void modbus_daemon(ModbusDaemonQueue &q,
                   vla::serial_io::OutputQueue::Sender outq,
                   RtuMessageHandler handle_indication) {
    ModbusDaemonFsm fsm(q, outq, handle_indication);
    while (true) {
        auto msg = q.receive();
        std::visit([&fsm](auto &event) { fsm.dispatch(event); }, msg);
    }
}

} // namespace vla
