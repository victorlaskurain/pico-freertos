#ifndef VLA_RTU_MESSAGE_HPP
#define VLA_RTU_MESSAGE_HPP

#include <cstdint>

namespace vla {

constexpr uint16_t PDU_MAX = 256;

struct RtuAddress {
    uint8_t address;
    explicit RtuAddress(uint8_t a) : address(a) {
    }
    bool operator==(RtuAddress other) {
        return address == other.address;
    }
    bool operator!=(RtuAddress other) {
        return address != other.address;
    }
};

enum class RtuFunctionCode : uint8_t {
    // physical discrete input
    READ_DISCRETE_INPUT = 0x02,
    // internal bits or physical bits
    READ_COILS        = 0x01,
    WRITE_SINGLE_COIL = 0x05,
    WRITE_COILS       = 0x0f,
    // physical input registers
    READ_INPUT_REGISTER = 0x04,
    // internal registers or physical output registers
    READ_HOLDING_REGISTERS        = 0x03,
    WRITE_SINGLE_REGISTER         = 0x06,
    WRITE_MULTIPLE_REGISTERS      = 0x10,
    READ_WRITE_MULTIPLE_REGISTERS = 0x17,
    MASK_WRITE_REGISTER           = 0x16,
    READ_FIFO_QUEUE               = 0x18,
    // file records
    READ_FILE_RECORD  = 0x14,
    WRITE_FILE_RECORD = 0x15,
    // diagnostics
    READ_EXCEPTION_STATUS      = 0x07,
    DIAGNOSTIC                 = 0x08,
    GET_COM_EVENT_COUNTER      = 0x0b,
    GET_COM_EVENT_LOG          = 0x0c,
    REPORT_SERVER_ID           = 0x11,
    READ_DEVICE_IDENTIFICATION = 0x2b
};

enum class RtuExceptionCode : uint8_t {
    ILLEGAL_FUNCTION                 = 0x01,
    ILLEGAL_DATA_ADDRESS             = 0x02,
    ILLEGAL_DATA_VALUE               = 0x03,
    SERVER_DEVICE_FAILURE            = 0x04,
    ACKNOLEDGE                       = 0x05,
    SERVER_DEVICE_BUSY               = 0x06,
    MEMORY_PARITY_ERROR              = 0x08,
    GATEWAY_PATH_UNAVAILABLE         = 0x0a,
    GATEWAY_TARGET_FAILED_TO_RESPOND = 0x0b
};

struct RtuMessage {
    uint8_t *buffer;
    uint8_t length;
    RtuMessage() = default;
    RtuMessage(uint8_t *b, uint8_t l) : buffer{b}, length{l} {
    }
    RtuAddress address() const {
        return RtuAddress{buffer[0]};
    }
    RtuFunctionCode function_code() const {
        return static_cast<RtuFunctionCode>(buffer[1]);
    }
};

} // namespace vla

#endif
