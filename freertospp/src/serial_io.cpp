#include <vla/serial_io.hpp>
#include <cstring>
#include <cstdlib>

namespace vla {
namespace serial_io {

BoxedCharPtr make_boxed_char_ptr(const char *s) {
    return BoxedCharPtr(ManagedCharPtr(strdup(s), &free));
}

} // namespace serial_io
} // namespace vla
