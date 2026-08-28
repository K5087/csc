#pragma once
#include <optional>
#include <vector>

namespace cmd {

// Cmd default is string_view, must have a valid string in outer
using Cmd = std::vector<std::string_view>;

struct Opt {
    bool wait_return = true;

    const char* fdin = nullptr;
    const char* fdot = nullptr;
    const char* fderr = nullptr;
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

struct Ret {
    impl::Proc proc;
    std::optional<int> value;
};

// escape string
std::string escape_string(std::string_view view);

// run an command
Ret run_cmd(const Cmd& cmd, Opt opt = {}) noexcept;

// wait procs until all return
void wait_procs(std::vector<Ret>& procs);

} // namespace cmd
