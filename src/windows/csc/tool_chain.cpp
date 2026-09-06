#include <csc/tool_chain.h>

namespace csc {

std::string_view ToolChain::GetCompiler() const {
    static std::string str = compiler.generic_string();
    return str;
}

std::string_view ToolChain::GetArchiver() const {
    static std::string str = archiver.generic_string();
    return str;
}

} // namespace csc
