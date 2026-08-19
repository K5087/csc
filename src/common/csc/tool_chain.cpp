#include <csc/tool_chain.h>

namespace csc {
LLVM::LLVM() {
    compiler = "clang++";
    archiver = "llvm-ar";
}

std::string_view get_default_compiler() {
#if defined(__clang__) && defined(_MSC_VER)
    return "clang-cl";
#elif defined(__clang__)
    return "clang++";
#elif defined(__GNUC__)
    return "g++";
#elif defined(_MSC_VER)
    return "cl";
#else
    throw "get compiler failed"
#endif
}
} // namespace csc
