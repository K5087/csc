#pragma once
#include <filesystem>

namespace csc {
using Path = std::filesystem::path;

class ToolChain {
public:
    Path preprocessor;
    Path parsing;
    Path ir_generation;
    Path compiler;
    Path assembler;
    Path linker;

    Path archiver;

public:
    std::string_view GetCompiler() const;
    std::string_view GetArchiver() const;

protected:
    ToolChain() = default;
};

class LLVM : public ToolChain {
public:
    LLVM();
};

// TODO: search tool chain?
std::string_view get_default_compiler();
std::string_view get_default_archiver();
std::shared_ptr<ToolChain> get_default_toolchain();
} // namespace csc
