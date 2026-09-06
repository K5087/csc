#pragma once
#include <source_location>
#include <stdexcept>

namespace rsc {
class dev_error : public std::runtime_error {
public:
    explicit dev_error(
        const std::string& message,
        std::source_location location = std::source_location::current());
};
} // namespace rsc
