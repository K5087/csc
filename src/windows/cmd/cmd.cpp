#include <cmd/cmd.h>
#include <log/log.h>
#include <windows.h>

namespace cmd {
std::string GetCommandStr(const Cmd& cmd) {
    std::string command;
    for (size_t i = 0; i < cmd.size(); i++) {
        std::string_view arg = cmd[i];
        if (i > 0) command.append(" ");
        if (need_escape(arg)) {
            command.append(escape_string(arg));
        } else {
            command.append(arg);
        }
    }
    return command;
}

namespace impl {
Proc create_proc(const Cmd& cmd, const Opt& opt) {
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    std::string command = GetCommandStr(cmd);
    BOOL result = CreateProcess(NULL, command.data(), NULL, NULL, TRUE, 0, NULL,
                                NULL, &si, &pi);

    if (!result) {
        error = GetLastError();
        return Proc(nullptr);
    }
    CloseHandle(pi.hThread);
    return Proc(pi.hProcess);
}

bool is_running(Proc proc) noexcept {
    if (!proc) {
        return false; // is must need?
    }
    DWORD result = WaitForSingleObject(proc, 0);
    if (result == WAIT_TIMEOUT) { return true; }

    return false;
}

std::optional<int> wait_proc(Proc proc) noexcept {
    DWORD result = WaitForSingleObject(proc, INFINITE);
    if (result == WAIT_FAILED) {
        error = GetLastError();
        return std::nullopt;
    }
    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        error = GetLastError();
        return std::nullopt;
    }

    CloseHandle(proc);
    return exit_status;
}

} // namespace impl
} // namespace cmd
