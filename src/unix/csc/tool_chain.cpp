#include <csc/tool_chain.h>

namespace csc {

std::string_view ToolChain::GetCompiler() const {
    return compiler.c_str();
}
} // namespace csc
