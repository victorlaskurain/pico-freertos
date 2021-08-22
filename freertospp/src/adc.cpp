#include <hardware/adc.h>
#include <hardware/irq.h>
#include <vla/adc.hpp>
#include <iostream>
#include <atomic>

namespace vla {
namespace adc {

static std::atomic<AdcMask> active_channels;

constexpr uint8_t ADC_PIN_BASE = 26;
constexpr uint16_t SAMPLE_ERROR_BIT = 1 << 12;

static uint16_t samples[CHANNEL_COUNT];

static uint8_t get_read_input(uint8_t next_input) {
    auto ac = active_channels.load();
    uint8_t read_input = (CHANNEL_COUNT - 1 + next_input) % CHANNEL_COUNT;
    while (!(ac & AdcInput(read_input)) && next_input != read_input) {
        read_input = (CHANNEL_COUNT - 1 + read_input) % CHANNEL_COUNT;
    }
    return read_input;
}

volatile uint32_t irq_count[CHANNEL_COUNT];
volatile uint32_t err_count[CHANNEL_COUNT];
static void on_adc_ready() {
    auto next_input = adc_get_selected_input();
    auto read_input = get_read_input(next_input);
    ++irq_count[read_input];
    uint16_t v = adc_fifo_get();
    if (!(SAMPLE_ERROR_BIT & v)) {
        samples[read_input] = v;
    } else {
        ++err_count[read_input];
    }
    adc_fifo_drain();
}

static void uninit() {
    auto ac = active_channels.load();
    if (ac) {
        adc_fifo_drain();
        adc_set_temp_sensor_enabled(false);
        adc_run(false);
        adc_irq_set_enabled(false);
        irq_set_enabled(ADC_IRQ_FIFO, false);
        adc_set_round_robin(0);
    }
}

static float frequency_to_divider(float hz) {
    return 48000000.0 / hz;
}

bool init(AdcMask ac, float frequency) {
    // cleanup any pending configuration
    uninit();
    active_channels = ac;
    auto active_channel_count = 0;
    for (uint8_t i = 0; i < CHANNEL_COUNT - 1; ++i) {
        const auto pin = ADC_PIN_BASE + i;
        if (ac & AdcInput(i)) {
            adc_gpio_init(pin);
            ++active_channel_count;
        }
    }
    adc_set_temp_sensor_enabled(ac & AdcInput::ADC_4);
    if (ac)  {
        adc_init();
        adc_set_round_robin(ac.mask);
        adc_irq_set_enabled(true);
        adc_fifo_setup(
            true,  // Write each completed conversion to the sample FIFO
            false, // Enable DMA data request (DREQ)
            1,     // DREQ (and IRQ) asserted when at least 1 sample present
            true, // Keep sample error bit on error
            false  // Keep full 12 bits of each sample
        );
        auto divider = frequency_to_divider(frequency * active_channel_count);
        adc_set_clkdiv(divider);
        irq_set_exclusive_handler(ADC_IRQ_FIFO, on_adc_ready);
        irq_set_enabled(ADC_IRQ_FIFO, true);
        adc_run(true);
    }
    return true;
}

std::optional<uint16_t> read(AdcInput channel) {
    if (!(channel & active_channels)) {
        return std::nullopt;
    }
    return samples[uint8_t(channel)];
}

std::ostream &operator<<(std::ostream &ost, AdcInput adci) {
    return ost << "AdcInput(" << int(adci) << ")";
}

std::ostream &operator<<(std::ostream &ost, AdcMask m) {
    return ost << "AdcMask(" << int(m.mask) << ")";
}

void test_f() {
    static_assert((AdcInput::ADC_0 | AdcInput::ADC_1).mask == 3);
    static_assert(AdcMask(AdcInput::ADC_4).mask == 16);
    static_assert((AdcInput::ADC_0 | AdcInput::ADC_4).mask == 17);
    static_assert((~AdcInput::ADC_0).mask == 0xfe);
}

} // namespace adc
} // namespace vla
