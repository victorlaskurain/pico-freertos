#ifndef VLA_MODBUS_DAEMON
#define VLA_MODBUS_DAEMON

#include <functional>
#include <variant>
#include <vla/rtu_message.hpp>
#include <vla/serial_io.hpp>
#include <vla/hw_timer.hpp>

namespace vla {

using RtuMessageHandler =
    std::function<void(const vla::RtuMessage &, vla::RtuMessage &)>;

struct TimeoutMsg {
    AlarmId aid;
    TimeoutMsg(AlarmId aid):aid(aid){}
};
struct ReadChar {
    uint64_t when;
    uint8_t chr;
    ReadChar() = default;
    explicit ReadChar(uint64_t w, uint8_t c) : when(w), chr(c) {
    }
};

using ModbusDaemonMessage =
    std::variant<ReadChar, TimeoutMsg, vla::RtuMessage, vla::serial_io::BytesWritten>;
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
