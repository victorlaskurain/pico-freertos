#include <FreeRTOS.h>
#include <functional>
#include <pico/stdlib.h>
#include <string.h>
#include <task.h>
#include <variant>

#include <vla/adc.hpp>
#include <vla/modbus_daemon.hpp>
#include <vla/pdu_handler_base.hpp>
#include <vla/queue.hpp>
#include <vla/serial_io.hpp>
#include <vla/task.hpp>

using vla::serial_io::OutputQueue;
auto output_manager = vla::serial_io::stdout_manager;

static void adc_init() {
    auto mask = vla::adc::AdcInput::ADC_0 | vla::adc::AdcInput::ADC_1 |
                vla::adc::AdcInput::ADC_4;
    vla::adc::init(mask, 100);
}

static void led_init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

struct Token {};
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

class RtuHandler : public vla::PduHandlerBase<RtuHandler> {
  public:
    bool is_read_registers_supported() {
        return true;
    }
    bool is_write_registers_supported() {
        return true;
    }
    bool execute_read_single_register(uint16_t address, uint16_t *w) {
        if (address <= uint16_t(vla::adc::AdcInput::ADC_4)) {
            auto adci = vla::adc::AdcInput(address);
            *w        = vla::adc::read(adci).value_or(-1);
        } else {
            *w = uint16_t(-1);
        }

        return true;
    }
    RtuHandler(vla::RtuAddress addr) : vla::PduHandlerBase<RtuHandler>(addr) {
    }
};

static auto rtu_handler = RtuHandler(vla::RtuAddress(0x01));
void handle_modbus_message(const vla::RtuMessage &indication,
                           vla::RtuMessage &reply) {
    rtu_handler.handle_indication(indication, reply);
}

int main() {
    // initialize libraries
    stdio_init_all();
    led_init();
    adc_init();
    // create a three task to keep the led blinking and show how can
    // several tasks talk to each other.
    auto sq  = SwitchQueue(1);
    auto tq1 = TokenQueue(1);
    auto tq2 = TokenQueue(1);
    tq1.send(Token());
    auto manager_task =
        vla::Task(std::bind(led_manager, sq.receiver()), "Led Manager", 256);
    auto led_on = vla::Task(
        std::bind(switch_led, SwitchLedConfig{.on        = true,
                                              .q         = sq.sender(),
                                              .token_in  = tq1.receiver(),
                                              .token_out = tq2.sender(),
                                              .delay_ms  = 100}),
        "Led on");
    auto led_off = vla::Task(
        std::bind(switch_led, SwitchLedConfig{.on        = false,
                                              .q         = sq.sender(),
                                              .token_in  = tq2.receiver(),
                                              .token_out = tq1.sender(),
                                              .delay_ms  = 1000}),
        "Led off");

    // create the stdout multiplexing task
    auto oq = OutputQueue(100);
    auto outputTask =
        vla::Task(std::bind(output_manager, oq.receiver()), "Output Task", 256);
    configASSERT(outputTask);

    // publish the most recent ADC value via modbus.
    auto modbus_task = vla::Task(
        std::bind(vla::modbus_daemon_stdin, oq.sender(), handle_modbus_message),
        "Modbus Task", 1024);

    vTaskStartScheduler();
    while (1) {
        configASSERT(0); /* We should never get here */
    }
}
