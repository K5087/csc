#include <rsc/dev.h>
#include <string>

namespace rsc {
dev_error::dev_error(const std::string& message, std::source_location location)
    : std::runtime_error(std::string(location.file_name()) + ": line " +
                         std::to_string(location.line()) + "\n" + message) {}

} // namespace rsc
