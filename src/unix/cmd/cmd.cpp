#include <cmd/cmd.h>
#include <log/log.h>
#include <sys/wait.h>

namespace cmd::impl {

Proc create_proc(const Cmd& cmd, const Opt& opt) {
    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork failed");
        return Proc(0);
    }
    if (cpid == 0) {
        std::vector<char*> argv;
        argv.reserve(cmd.size() + 1);
        for (auto& s : cmd) {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        perror("execvp");
        _exit(127);
    }
    return cpid;
}

bool is_running(Proc proc) noexcept {
    int   status = 0;
    pid_t result = waitpid(proc, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (result == 0)
        return true;
    else if (result < 0)
        return false;
    if (WIFEXITED(status) || WIFSIGNALED(status))
        return false;
    else if (WIFSTOPPED(status) || WIFCONTINUED(status))
        return true;
    return true;
}

std::optional<int> wait_proc(Proc proc) noexcept {
    int status = 0;
    if (waitpid(proc, &status, 0) < 0) {
        error = errno;
        return std::nullopt;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return std::nullopt;
}
} // namespace cmd::impl
