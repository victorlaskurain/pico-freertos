#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <string.h>
#include <task.h>
#include <unistd.h>
#include <variant>

#include <vla/adc.hpp>
#include <vla/queue.hpp>
#include <vla/task.hpp>

extern "C" void quick_blink(const int n) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(250);
    for (auto i = 0; i < n; ++i) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(100);
    }
    sleep_ms(250);
}

using ManagedCharPtr = std::unique_ptr<char, decltype(&free)>;
using BoxedCharPtr   = vla::Box<ManagedCharPtr>;
BoxedCharPtr make_boxed_char_ptr(const char *s) {
    return BoxedCharPtr(ManagedCharPtr(strdup(s), &free));
}
/**
 * OutputMsg is a message to the outputManager. It can be an owned
 * charptr in the form of a Boxed unique_ptr or a simple non owning
 * char pointer.
 */
struct OutputMsg : public vla::WithReply<int32_t> {
    std::variant<BoxedCharPtr, const char *> buffer;
    OutputMsg() = default;
    template <typename B, typename ReplyQueue>
    OutputMsg(B &&b, ReplyQueue &q) : WithReply(q), buffer(b) {
    }
    template <typename B> OutputMsg(B &&b) : buffer(b) {
    }
};
using OutputQueue = vla::Queue<OutputMsg>;

void outputManager(OutputQueue::Receiver q) {
    stdio_init_all();
    while (true) {
        auto msg = q.receive();
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

#include <sstream>

template <typename Param1, typename... Params>
void print2(OutputQueue::Sender q, std::ostringstream &ost, Param1 p,
            Params... rest) {
    ost << p;
    print2(q, ost, rest...);
}
void print2(OutputQueue::Sender q, std::ostringstream &ost) {
    vla::Queue<int32_t> outAck(1);
    q.send({ost.str().c_str(), outAck});
    outAck.receiver().receive();
}

template <typename... Params>
void print(OutputQueue::Sender q, Params... params) {
    std::ostringstream ost;
    print2(q, ost, params...);
}

namespace vla {
namespace adc {
uint8_t get_read_input(uint8_t next_input);
extern volatile uint32_t irq_count[CHANNEL_COUNT];
} // namespace adc
} // namespace vla
void run(OutputQueue::Sender q) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    for (auto i = 5; i > 0; --i) {
        print(q, "Sleep ", i, "\n");
        sleep_ms(1000);
    }
    print(q, "Begin\n");
    auto mask = vla::adc::AdcInput::ADC_0 | vla::adc::AdcInput::ADC_1 |
                vla::adc::AdcInput::ADC_4;
    vla::adc::init(mask, 100);
    while (true) {
        for (auto adci : {vla::adc::AdcInput::ADC_0, vla::adc::AdcInput::ADC_1,
                          vla::adc::AdcInput::ADC_2, vla::adc::AdcInput::ADC_3,
                          vla::adc::AdcInput::ADC_4}) {
            print(q, vla::adc::irq_count[uint8_t(adci)], "\n");
            print(q, "Selected: ", adci,
                  " value: ", vla::adc::read(adci).value_or(-1), "\n");
        }
        print(q, "\n");
        quick_blink(1);
        /*
        print(q, "Selected: ", vla::adc::AdcInput::ADC_4, "\n");
        auto v = vla::adc::read(vla::adc::AdcInput::ADC_4).value_or(-1);
        double ADC_voltage = double(v) * 3.3 / 4095.0;
        print(q, "Value: ", v, " (", 27.0 - (ADC_voltage - 0.706) / 0.001721,
              ")", "\n\n");
        */
        sleep_ms(1000);
    }
}

int main() {
    auto oq = OutputQueue(1);
    stdio_init_all();
    // quick_blink(2);

    auto mainTask = vla::Task(std::bind(run, oq.sender()), "Main Task", 1024);
    configASSERT(mainTask);

    auto outputTask =
        vla::Task(std::bind(outputManager, oq.receiver()), "Output Task", 1024);
    configASSERT(outputTask);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
