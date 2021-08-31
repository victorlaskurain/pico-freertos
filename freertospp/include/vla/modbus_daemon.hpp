#ifndef VLA_MODBUS_DAEMON
#define VLA_MODBUS_DAEMON

#include <functional>
#include <variant>
#include <vla/pdu_handler_base.hpp> // :TODO: refactor this to include only rtu_message definition
#include <vla/serial_io.hpp>

namespace vla {

using RtuMessageHandler =
    std::function<void(const vla::rtu_message &, vla::rtu_message &)>;

struct TimeoutMsg {};
struct ReadChar {
    uint64_t when;
    uint8_t chr;
    ReadChar() = default;
    explicit ReadChar(uint64_t w, uint8_t c) : when(w), chr(c) {
    }
};

using ModbusDaemonMessage =
    std::variant<ReadChar, TimeoutMsg, vla::rtu_message, int32_t>;
using ModbusDaemonQueue = vla::Queue<ModbusDaemonMessage>;

// functions of this type are responsible for feeding chars (ReadChar)
using GetCharsCb = void (*)(ModbusDaemonQueue::SenderIsr *);

void modbus_daemon(ModbusDaemonQueue &q,
                   vla::serial_io::OutputQueue::Sender outq,
                   RtuMessageHandler handle_indication);

void modbus_daemon_stdin(vla::serial_io::OutputQueue::Sender outq,
                         RtuMessageHandler handle_indication);

} // namespace vla

#endif // VLA_MODBUS_DAEMON
