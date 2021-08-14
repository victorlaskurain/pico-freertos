#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <string.h>
#include <task.h>
#include <unistd.h>

#include <vla/queue.hpp>
#include <vla/task.hpp>
#include <variant>

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
    static Buffer createFromString(const char *str) {
        return {
            .data = (uint8_t *)str, .size = strlen(str), .deleter = nullptr};
    }
};
using ManagedCharPtr = std::unique_ptr<char, decltype(&free)>;
using BoxedCharPtr = vla::Box<ManagedCharPtr>;
BoxedCharPtr make_boxed_char_ptr(const char* s) {
    return BoxedCharPtr(ManagedCharPtr(strdup(s), &free));
}
/**
 * OutputMsg is a message to the outputManager. It can be a Buffer, an
 * owed charptr in the form of a Boxed unique_ptr or a simple non
 * owning char pointer
 */
struct OutputMsg : public vla::WithReply<int32_t> {
    std::variant<Buffer, BoxedCharPtr, const char*> buffer;
    OutputMsg() = default;
    template <typename B, typename ReplyQueue>
    OutputMsg(B &&b, ReplyQueue &q) : WithReply(q), buffer(b) {}
    template<typename B>
    OutputMsg(B &&b) : buffer(b){}
};
struct InputMsg : public vla::WithReply<Buffer> {
    Buffer buffer;
    InputMsg() = default;
    template <typename ReplyQueue>
    InputMsg(const Buffer &b, ReplyQueue &q) : WithReply(q), buffer(b) {}
};
using InputQueue = vla::Queue<InputMsg>;
using OutputQueue = vla::Queue<OutputMsg>;

void inputManager(InputQueue::Receiver q) {
    stdio_init_all();
    while (true) {
        auto msg = q.receive();
        if (msg.buffer.size) {
            msg.buffer.size = read(0, msg.buffer.data, msg.buffer.size);
            msg.reply(msg.buffer);
        }
    }
}

void outputManager(OutputQueue::Receiver q) {
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
        if (auto s = std::get_if<const char*>(&msg.buffer)) {
            msg.reply(write(1, *s, strlen(*s)));
        }
    }
}

using BlinkQueue = vla::Queue<bool>;
void blink(BlinkQueue::Receiver q) {
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    while (true) {
        bool switchOn;
        q.receive(switchOn);
        if (switchOn) {
            gpio_put(LED_PIN, 1);
        } else {
            gpio_put(LED_PIN, 0);
        }
    }
}

void echo(InputQueue::Sender iq, OutputQueue::Sender oq) {
    vla::Queue<Buffer> reader(1);
    vla::Queue<int32_t> outAck(1);
    uint8_t data[16];
    while (true) {
        // 1. Send the read request. We can ignore the reply since it will be
        //    written in the buffer we provided.
        // 2. Send the write message (echo) and wait for ack since we reuse the
        //    buffer.
        auto buff = Buffer::create(data, sizeof(data));
        iq.send(InputMsg(buff, reader));
        reader.receiver().receive();
        if (buff.size) {
            oq.send(OutputMsg(buff, outAck));
            outAck.receiver().receive();
        }
    }
}

void run(BlinkQueue::Sender &bq, OutputQueue::Sender &oq) {
    bool isLedOn = false;
    while (true) {
        vTaskDelay(1000);
        isLedOn = !isLedOn;
        bq.send(isLedOn);
        // three calls just to show different message body types.
        oq.send(Buffer::createFromString("Toggle"));
        oq.send(make_boxed_char_ptr(" "));
        oq.send("led!\n");
    }
}

int main() {
    auto blinkyq = BlinkQueue(1);
    auto oq = OutputQueue(1);
    auto iq = InputQueue(1);

    auto blinky =
        vla::Task(std::bind(blink, blinkyq.receiver()), "Blinky task");
    configASSERT(blinky);

    auto writer =
        vla::Task(std::bind(outputManager, oq.receiver()), "Output Task");
    configASSERT(writer);

    auto reader =
        vla::Task(std::bind(inputManager, iq.receiver()), "Input Task");
    configASSERT(reader);

    auto echoTask =
        vla::Task(std::bind(echo, iq.sender(), oq.sender()), "Echo Task");
    configASSERT(echoTask);

    auto mainTask =
        vla::Task(std::bind(run, blinkyq.sender(), oq.sender()), "Main Task");
    configASSERT(mainTask);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
