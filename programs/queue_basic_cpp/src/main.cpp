#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <string.h>
#include <task.h>
#include <unistd.h>

#include <variant>
#include <vla/queue.hpp>
#include <vla/serial_io.hpp>
#include <vla/task.hpp>

using vla::serial_io::Buffer;
using ManagedCharPtr = std::unique_ptr<char, decltype(&free)>;
using BoxedCharPtr   = vla::Box<ManagedCharPtr>;
BoxedCharPtr make_boxed_char_ptr(const char *s) {
    return BoxedCharPtr(ManagedCharPtr(strdup(s), &free));
}
using InputMsg    = vla::serial_io::InputMsg;
using OutputMsg   = vla::serial_io::OutputMsg;
using InputQueue  = vla::serial_io::InputQueue;
using OutputQueue = vla::serial_io::OutputQueue;

auto inputManager  = vla::serial_io::stdin_manager;
auto outputManager = vla::serial_io::stdout_manager;

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

Buffer buffer_from_string(const char *str) {
    return {.data = (uint8_t *)str, .size = strlen(str), .deleter = nullptr};
}

void run(BlinkQueue::Sender &bq, OutputQueue::Sender &oq) {
    bool isLedOn = false;
    while (true) {
        vTaskDelay(1000);
        isLedOn = !isLedOn;
        bq.send(isLedOn);
        // three calls just to show different message body types.
        oq.send(buffer_from_string("Toggle"));
        oq.send(make_boxed_char_ptr(" "));
        oq.send("led!\n");
    }
}

int main() {
    auto blinkyq = BlinkQueue(1);
    auto oq      = OutputQueue(1);
    auto iq      = InputQueue(1);

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
