#include <csc/target.h>

namespace csc {
std::string_view get_extension(TargetType type, TargetFlavor flavor) {
    switch (type) {
        case TargetType::exec: return ".exe";
        case TargetType::arch:
            switch (flavor) {
                case TargetFlavor::GNU: return ".a";
                case TargetFlavor::MSVC: return ".lib";
            };
        case TargetType::share: return ".dll";
    }
}
} // namespace csc
