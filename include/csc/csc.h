#pragma once
#include <cmd/cmd.h>
#include <csc/target.h>

#include <filesystem>
#include <vector>

namespace csc {
using Path = std::filesystem::path;

bool update_bin(Path bin, const std::vector<Path>& files, const cmd::Cmd& cmd);
/**
 * @brief if argv[0] need update,use cmd update
 *
 * @param argc input arg count
 * @param argv input args
 * @param files this files will diff date with argv[0]
 * @param cmd update command
 * @return true as update success,false is update failed or no need to update
 */
bool update_self(int argc, char** argv, std::vector<Path>& files, cmd::Cmd cmd = {});

/**
 * @brief Check whether a file is out of date.
 *
 * @param file The file to check.
 * @param inputs Files whose modification times are compared against @p file.
 * @return true if @p file is out of date; false otherwise.
 */
bool is_outdated(const Path& file, const std::vector<Path>& inputs);

std::string_view get_extension();

bool build_target(std::shared_ptr<Target> info);
cmd::Ret compile_unit(const Target& info, const Path& unit, const std::string& obj);
bool link_exe(Target& info, const std::vector<std::string>& objs);
bool link_lib(Target& info, const std::vector<std::string>& objs);
bool link_dll(Target& info, const std::vector<std::string>& objs);

namespace impl {

}
} // namespace csc
