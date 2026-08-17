#include <cmd/cmd.h>

#include <log/log.h>

#include <chrono>
#include <semaphore>
#include <thread>

namespace cmd {

static std::vector<impl::Proc>   procs;
static std::counting_semaphore<> sem(std::thread::hardware_concurrency() + 1);

namespace impl {

bool check_procs() {
    bool result = false;
    for (auto iter = procs.begin(); iter != procs.end();) {
        if (impl::is_running(*iter)) {
            iter = procs.erase(iter);
            sem.release();
            result = true;
        } else {
            iter++;
        }
    }
    return result;
}
} // namespace impl

std::optional<int> run_cmd(const Cmd& cmd, Opt opt) noexcept {
    if (cmd.empty()) {
        loge("Could not run empty command");
        return std::nullopt;
    }

    impl::check_procs();

    while (!sem.try_acquire()) {
        logi("cmd process num too much,wait someone quit");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        impl::check_procs();
    }

    impl::Proc proc = impl::create_proc(cmd, opt);

    if (!proc) {
        sem.release();
        return std::nullopt;
    }

    if (opt.wait_return) {
        auto ret = impl::wait_proc(proc);
        sem.release();
        return ret;
    } else {
        procs.push_back(proc);
    }

    return 0;
}

} // namespace cmd
