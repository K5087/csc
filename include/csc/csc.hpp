#pragma once
#include <csc/csc.h>
#include <csc/target.h>
#include <csc/tool.h>

namespace csc {

#define QUOTE_IMPL(...) #__VA_ARGS__
#define QUOTE(...)      QUOTE_IMPL(__VA_ARGS__)

#ifdef CSC_INCLUDE_DIR
inline std::vector<fs::path> get_predefine_include() { return CSC_INCLUDE_DIR; }
#endif

#ifdef CSC_LINK_SHARED_LIB
inline std::vector<fs::path> get_predefine_shared() {
    return CSC_LINK_SHARED_LIB;
}
#endif

#ifdef CSC_LINK_STATIC_LIB
inline std::vector<fs::path> get_predefine_static() {
    return CSC_LINK_STATIC_LIB;
}
#endif

inline std::vector<std::string> get_predefine_macro() {
    std::vector<std::string> macros;

#ifdef CSC_INCLUDE_DIR
    macros.emplace_back(std::string("-CSC_INCLUDE_DIR=") +
                        QUOTE(CSC_INCLUDE_DIR));
#endif
#ifdef CSC_LINK_SHARED_LIB
    macros.emplace_back(std::string("-DCSC_LINK_SHARED_LIB=") +
                        QUOTE(CSC_LINK_SHARED_LIB));
#endif
#ifdef CSC_LINK_STATIC_LIB
    macros.emplace_back(std::string("-CSC_LINK_STATIC_LIB=") +
                        QUOTE(CSC_LINK_STATIC_LIB));
#endif

    return macros;
}

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
        std::vector<fs::path> inputs = files;
        std::vector<fs::path> includes{};
#ifdef CSC_INCLUDE_DIR
        includes = get_predefine_include();
#endif
#ifdef CSC_LINK_SHARED_LIB
        inputs.append_range(get_predefine_shared());
#endif
#ifdef CSC_LINK_STATIC_LIB
        inputs.append_range(get_predefine_static());
#endif
        auto command = make_compile_cmd(inputs, includes, bin);
        temp.append_range(command);
        auto macros = get_predefine_macro();
        temp.append_range(macros);

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
