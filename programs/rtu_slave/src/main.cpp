#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <string.h>
#include <task.h>
#include <unistd.h>
#include <variant>

#include <vla/adc.hpp>
#include <vla/modbus_daemon.hpp>
#include <vla/pdu_handler_base.hpp>
#include <vla/queue.hpp>
#include <vla/serial_io.hpp>
#include <vla/task.hpp>

using vla::serial_io::BoxedCharPtr;
using vla::serial_io::InputQueue;
using vla::serial_io::make_boxed_char_ptr;
using vla::serial_io::ManagedCharPtr;
using vla::serial_io::OutputMsg;
using vla::serial_io::OutputQueue;
auto output_manager = vla::serial_io::stdout_manager;
auto input_manager = vla::serial_io::stdin_manager;

// using vla::serial_io::print;
#define print(...)

static void run(OutputQueue::Sender q) {
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
            print(q, "Selected: ", adci, " value: ", vla::adc::read(adci).value_or(-1), "\n");
        }
        print(q, "\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void led_init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

struct Token{int32_t ignore;};
using TokenQueue  = vla::Queue<Token>;
using SwitchQueue = vla::Queue<bool>;
static void led_manager(SwitchQueue::Receiver q) {
    led_init();
    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, q.receive());
    }
}

struct SwitchLedConfig {
    bool on;
    SwitchQueue::Sender q;
    TokenQueue::Receiver token_in;
    TokenQueue::Sender token_out;
    uint32_t delay_ms;
};
static void switch_led(SwitchLedConfig c) {
    while (true) {
        c.token_in.receive();
        c.q.send(c.on);
        vTaskDelay(c.delay_ms / portTICK_PERIOD_MS);
        c.token_out.send(Token());
    }
}

static void write_msg(OutputQueue::Sender q, const char *msg, uint16_t initial_delay) {
    auto i = 0;
    sleep_ms(initial_delay);
    while (true) {
        print(q, ++i, " ", msg, "\n");
        sleep_ms(1000);
    }
}

class RtuHandler : public vla::pdu_handler_base<RtuHandler> {
  public:
    bool is_read_registers_supported() { return true; }
    bool is_write_registers_supported() { return true; }
    bool execute_read_single_register(uint16_t address, uint16_t *w) {
        *w = address;
        return true;
    }
    RtuHandler(vla::rtu_address addr)
        : vla::pdu_handler_base<RtuHandler>(addr) {}
};

static auto rtu_handler = RtuHandler(vla::rtu_address(0x01));
void handle_modbus_message(const vla::rtu_message &indication,
                           vla::rtu_message &reply) {
    rtu_handler.handle_indication(indication, reply);
}

int main() {
    stdio_init_all();
    led_init();
    auto sq = SwitchQueue(1);
    auto tq1 = TokenQueue(1);
    auto tq2 = TokenQueue(1);
    tq1.send(Token());
    auto manager_task =
        vla::Task(std::bind(led_manager, sq.receiver()), "Led Manager", 256);
    auto led_on = vla::Task(
        std::bind(switch_led, SwitchLedConfig{.on = true,
                                              .q = sq.sender(),
                                              .token_in = tq1.receiver(),
                                              .token_out = tq2.sender(),
                                              .delay_ms = 100}),
        "Led on");
    auto led_off = vla::Task(
        std::bind(switch_led, SwitchLedConfig{.on = false,
                                              .q = sq.sender(),
                                              .token_in = tq2.receiver(),
                                              .token_out = tq1.sender(),
                                              .delay_ms = 1000}),
        "Led off");

    auto oq = OutputQueue(100);
    auto iq = InputQueue(8);

    auto mainTask = vla::Task(std::bind(run, oq.sender()), "Main Task", 512);
    configASSERT(mainTask);

    auto pingTask = vla::Task(std::bind(write_msg, oq.sender(), "Ping!", 0), "Ping Task", 256);
    configASSERT(pingTask);

    auto pongTask = vla::Task(std::bind(write_msg, oq.sender(), "Pong!", 250), "Pong Task", 256);
    configASSERT(pongTask);

    auto outputTask = vla::Task(std::bind(output_manager, oq.receiver()),
                                "Output Task", 256);
    configASSERT(outputTask);

    auto inputTask =
        vla::Task(std::bind(input_manager, iq.receiver()), "Input Task", 256);

    auto modbus_task = vla::Task(std::bind(vla::modbus_daemon_stdin,
                                           oq.sender(), handle_modbus_message),
                                 "Modbus Task", 1024);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
