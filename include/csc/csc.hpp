#pragma once
#include <csc/csc.h>
#include <csc/target.h>
#include <csc/tool.h>

namespace csc {

fs::path get_predefine_include();
fs::path get_predefine_shared();
std::vector<fs::path> get_predefine_static();
std::vector<std::string> get_predefine_macro();

#ifdef CSC_INSTALL_DIR
inline std::string quote(std::string_view s) {
    return "\"" + std::string(s) + "\"";
}

inline fs::path get_predefine_include() {
    return fs::path(CSC_INSTALL_DIR) / "include";
}

    #ifdef CSC_LINK_SHARED_LIB
inline fs::path get_predefine_shared() {
    return fs::path(CSC_INSTALL_DIR) / "lib/shared/" / CSC_LINK_SHARED_LIB;
}
    #endif

inline std::vector<fs::path> get_predefine_static() {
    fs::path static_dir = fs::path(CSC_INSTALL_DIR) / "lib/static";
    return {(static_dir / "log.a"), (static_dir / "cmd.a"),
            (static_dir / "argp.a"), (static_dir / "csc.a")};
}

inline std::vector<std::string> get_predefine_macro() {
    std::vector<std::string> macros;
    macros.emplace_back(std::string("-DCSC_INSTALL_DIR=") +
                        quote(CSC_INSTALL_DIR));

    #ifdef CSC_LINK_SHARED_LIB
    macros.emplace_back(std::string("-DCSC_LINK_SHARED_LIB=") +
                        quote(CSC_LINK_SHARED_LIB));
    #endif
    return macros;
}
#endif

/**
 * @brief if argv[0] need update,use cmd update
 *
 * @param argc input arg count
 * @param argv input args
 * @param files this files will diff date with argv[0]
 * @param cmd update command(if empty, use files as compile input)
 * @param exec_new when update success,whether exec new bin(old will exit)
 * @param fail_exit when update failed,whether exit
 */
inline void update_self(int argc, char** argv,
                        const std::vector<fs::path>& files,
                        const cmd::Cmd& cmd = {}, bool exec_new = true,
                        bool fail_exit = true) {
    fs::path bin = argv[0];
    UpdateStatus status = UpdateStatus::failed;
    if (cmd.empty()) {
        cmd::Cmd temp;
#ifdef CSC_INSTALL_DIR
        std::vector<fs::path> inputs = files;
    #ifdef CSC_LINK_SHARED_LIB
        inputs.emplace_back(get_predefine_shared());
    #else
        inputs.append_range(get_predefine_static());
    #endif
        auto command = make_compile_cmd(inputs, {get_predefine_include()}, bin);
        temp.append_range(command);
        auto macros = get_predefine_macro();
        temp.append_range(macros);
#else
        auto command = make_compile_cmd(files, {}, bin);
        temp.append_range(command);
#endif

        status = update_bin(bin, files, temp);
    } else {
        status = update_bin(bin, files, cmd);
    }
    switch (status) {
        case UpdateStatus::success:
            if (exec_new) {
                cmd::Cmd exec;
                for (int i = 0; i < argc; i++) { exec.push_back(argv[i]); }
                cmd::run_cmd(exec);

                // no matter exit is what,when exec_new, old alwase exit
                std::exit(0);
            }
            break;
        case csc::UpdateStatus::noneed: break;
        case csc::UpdateStatus::failed:
            if (fail_exit) { std::exit(0); }
            break;
    }
}
} // namespace csc
