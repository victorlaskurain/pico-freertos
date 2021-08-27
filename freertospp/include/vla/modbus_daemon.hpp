#ifndef VLA_MODBUS_DAEMON
#define VLA_MODBUS_DAEMON

#include <functional>
#include <vla/serial_io.hpp>
#include <vla/pdu_handler_base.hpp> // :TODO: refactor this to include only rtu_message definition

namespace vla {

using RtuMessageHandler =
    std::function<void(const vla::rtu_message &, vla::rtu_message &)>;

void modbus_daemon(vla::serial_io::InputQueue::Sender inq,
                   vla::serial_io::OutputQueue::Sender outq,
                   RtuMessageHandler handle_indication);

} // namespace vla

#endif // VLA_MODBUS_RTU_SLAVE_MODBUS_DAEMON
