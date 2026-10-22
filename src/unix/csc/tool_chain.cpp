#include <csc/tool_chain.h>

namespace csc {

std::string_view ToolChain::GetCompiler() const { return compiler.c_str(); }

std::string_view ToolChain::GetArchiver() const { return archiver.c_str(); }

std::string_view ToolChain::GetScaner() const { return scanner.c_str(); }
} // namespace csc
