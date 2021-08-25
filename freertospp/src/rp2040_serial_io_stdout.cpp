#include <vla/serial_io.hpp>
#include <cstring>
#include <pico/stdlib.h>
#include <unistd.h>

namespace vla {
namespace serial_io {

void stdout_manager(OutputQueue::Receiver q) {
    stdio_init_all();
    while (true) {
        auto msg = q.receive();
        if (auto buffer = std::get_if<Buffer>(&msg.buffer)) {
            if (buffer->size) {
                msg.reply(write(1, buffer->data, buffer->size));
                if (buffer->deleter) {
                    buffer->deleter(buffer->data);
                }
            }
            continue;
        }
        if (auto box = std::get_if<BoxedCharPtr>(&msg.buffer)) {
            auto chars = box->unbox();
            msg.reply(write(1, chars.get(), strlen(chars.get())));
            continue;
        }
        if (auto s = std::get_if<const char *>(&msg.buffer)) {
            msg.reply(write(1, *s, strlen(*s)));
        }
    }
}

void stdin_manager(InputQueue::Receiver q) {
    stdio_init_all();
    while (true) {
        auto msg = q.receive();
        if (msg.buffer.size) {
            msg.buffer.size = read(0, msg.buffer.data, msg.buffer.size);
            msg.reply(msg.buffer);
        }
    }
}

} // namespace serial_io
} // namespace vla
