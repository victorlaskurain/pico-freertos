#ifndef VLA_ADC_HPP
#define VLA_ADC_HPP

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace vla {
namespace adc {

enum class AdcInput : uint8_t { ADC_0, ADC_1, ADC_2, ADC_3, ADC_4 };

std::ostream &operator<<(std::ostream &ost, AdcInput adci);

constexpr uint8_t CHANNEL_COUNT = 5;

struct AdcMask {
    uint8_t mask;
    AdcMask() = default;
    constexpr AdcMask(AdcInput ai) : mask(1 << uint8_t(ai)) {}
    constexpr AdcMask &operator|=(AdcMask right) {
        mask |= right.mask;
        return *this;
    }
    constexpr AdcMask &operator&=(AdcMask right) {
        mask &= right.mask;
        return *this;
    }
    operator bool() const { return mask; }
};

std::ostream &operator<<(std::ostream &ost, AdcMask m);

constexpr AdcMask operator~(AdcMask left) {
    left.mask = ~left.mask;
    return left;
}

constexpr AdcMask operator~(AdcInput left) { return ~AdcMask(left); }

constexpr AdcMask operator|(AdcMask left, AdcInput right) {
    return left |= right;
}

constexpr AdcMask operator|(AdcInput left, AdcMask right) {
    return right | left;
}

constexpr AdcMask operator|(AdcInput left, AdcInput right) {
    return AdcMask(left) | right;
}

constexpr AdcMask operator&(AdcMask left, AdcInput right) {
    return left &= right;
}

constexpr AdcMask operator&(AdcInput left, AdcMask right) {
    return right & left;
}

constexpr AdcMask operator&(AdcInput left, AdcInput right) {
    return AdcMask(left) & right;
}

bool init(AdcMask active_channels, float frequency_hz);

std::optional<uint16_t> read(AdcInput channel);

} // namespace adc
} // namespace vla

#endif
