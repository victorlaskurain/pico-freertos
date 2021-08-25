#ifndef VLA_SERIAL_IO_HPP
#define VLA_SERIAL_IO_HPP

#include <cstdint>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <variant>

#include <vla/queue.hpp>

namespace vla {
namespace serial_io {

struct Buffer {
    using Deleter = void (*)(uint8_t *);
    uint8_t *data;
    uint32_t size;
    Deleter deleter;
    static Buffer createOwned(uint8_t *data, uint32_t size, Deleter deleter) {
        return {.data = data, .size = size, .deleter = deleter};
    }
    static Buffer create(uint8_t *data, uint32_t size) {
        return {.data = data, .size = size, .deleter = nullptr};
    }
};

using ManagedCharPtr = std::unique_ptr<char, decltype(&free)>;
using BoxedCharPtr = vla::Box<ManagedCharPtr>;
BoxedCharPtr make_boxed_char_ptr(const char *s);
/**
 * OutputMsg is a message to the outputManager. It can be an owned
 * charptr in the form of a Boxed unique_ptr or a simple non owning
 * char pointer.
 */
struct OutputMsg : public vla::WithReply<int32_t> {
    std::variant<BoxedCharPtr, Buffer, const char *> buffer;
    OutputMsg() = default;
    template <typename B, typename ReplyQueue>
    OutputMsg(B &&b, ReplyQueue &q) : WithReply(q), buffer(b) {}
    template <typename B> OutputMsg(B &&b) : buffer(b) {}
};
using OutputQueue = vla::Queue<OutputMsg>;

template <typename Param1, typename... Params>
void print2(OutputQueue::Sender q, std::ostringstream &ost, Param1 p,
            Params... rest) {
    ost << p;
    print2(q, ost, rest...);
}
inline void print2(OutputQueue::Sender q, std::ostringstream &ost) {
    vla::Queue<int32_t> outAck(1);
    q.send({ost.str().c_str(), outAck});
    outAck.receiver().receive();
}

template <typename... Params>
void print(OutputQueue::Sender q, Params... params) {
    std::ostringstream ost;
    print2(q, ost, params...);
}

void stdout_manager(OutputQueue::Receiver q);

struct InputMsg : public vla::WithReply<Buffer> {
    Buffer buffer;
    InputMsg() = default;
    template <typename ReplyQueue>
    InputMsg(const Buffer &b, ReplyQueue &q) : WithReply(q), buffer(b) {}
};
using InputQueue = vla::Queue<InputMsg>;

void stdin_manager(InputQueue::Receiver q);

} // namespace serial_io
} // namespace vla

#endif
