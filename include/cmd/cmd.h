#pragma once
#include <optional>
#include <string>
#include <vector>

namespace cmd {

using Cmd = std::vector<std::string>;

struct Opt {
    const char* fdin        = nullptr;
    const char* fdot        = nullptr;
    const char* fderr       = nullptr;
    bool        wait_return = true;
};
#ifdef _WIN32
inline thread_local unsigned long error;
#else
inline thread_local int error;
#endif // _WIN32

namespace impl {

#ifdef _WIN32
using Proc = void*;
#else
using Proc = int;
#endif // _WIN32

// create a process to run command
Proc create_proc(const Cmd& cmd, const Opt& opt);

// check process is running
bool is_running(Proc proc) noexcept;

// wait until process return
std::optional<int> wait_proc(Proc proc) noexcept;

} // namespace impl

// run an command
std::optional<int> run_cmd(const Cmd& cmd, Opt opt = {}) noexcept;

} // namespace cmd
