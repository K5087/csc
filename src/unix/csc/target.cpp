#include <csc/target.h>

namespace csc {
std::string_view get_extension(TargetType type, TargetFlavor flavor) {
    switch (type) {
        case TargetType::exec: return ".exe";
        case TargetType::arch: return ".a";
        case TargetType::share: return ".so";
    }
}
} // namespace csc
