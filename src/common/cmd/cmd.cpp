#include <cmd/cmd.h>

#include <log/log.h>

#include <chrono>
#include <semaphore>
#include <thread>

namespace cmd {

static std::vector<impl::Proc> procs;
static std::counting_semaphore<> sem(std::thread::hardware_concurrency() + 1);
#ifdef _WIN32
    #define NULL_PROC nullptr
#else
    #define NULL_PROC 0
#endif // _WIN32

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

bool need_escape(std::string_view view) {
    return view.empty() ||
           std::string_view::npos != view.find_first_of(" \t\n\v\"");
}

std::string escape_string(std::string_view view) {
    std::string str("\"");
    size_t backslashes = 0;
    for (size_t j = 0; j < view.length(); ++j) {
        switch (view[j]) {
            case '\\': backslashes += 1; break;
            case '\"':
                str.append(2 * backslashes + 1, '\\');
                backslashes = 0;
                str.push_back(view[j]);
                break;
            default:
                str.append(backslashes, '\\');
                backslashes = 0;
                str.push_back(view[j]);
                break;
        }
    }
    str.append(2 * backslashes, '\\');
    str.append("\"");
    return str;
}

Ret run_cmd(const Cmd& cmd, Opt opt) noexcept {
    if (cmd.empty()) {
        loge("Could not run empty command");
        return {NULL_PROC, std::nullopt};
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
        return {NULL_PROC, std::nullopt};
    }

    if (opt.wait_return) {
        auto ret = impl::wait_proc(proc);
        sem.release();
        return {proc, ret};
    } else {
        procs.push_back(proc);
    }

    return {proc, 0};
}

void wait_procs(std::vector<Ret>& rets) {
    for (auto& ret : rets) { ret.value = impl::wait_proc(ret.proc); }
}

} // namespace cmd
