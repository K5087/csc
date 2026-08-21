#pragma once
#include <cmd/cmd.h>

#include <filesystem>
#include <vector>

namespace csc {
class Target;
namespace fs = std::filesystem;

enum class UpdateStatus { success, noneed, failed };

/**
 * @brief update bin by cmd
 *
 * @param bin binary file path
 * @param files which files will be compare with bin's date
 * @param cmd commands to update
 * @return success noneed failed
 */
UpdateStatus update_bin(fs::path bin, const std::vector<fs::path>& files,
                        const cmd::Cmd& cmd);
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
void update_self(int argc, char** argv, const std::vector<fs::path>& files,
                 const cmd::Cmd& cmd = {}, bool exec_new = true,
                 bool fail_exit = true);

/**
 * @brief Check whether a file is out of date.
 *
 * @param file The file to check.(should be latest)
 * @param inputs Files whose modification times are compared against @p file.
 * @return true if @p file is out of date; false otherwise.
 */
bool is_outdated(const std::filesystem::path& file,
                 const std::vector<fs::path>& inputs);

std::string_view get_extension();

bool build_target(std::shared_ptr<Target> target);

std::vector<std::string> make_compile_cmd(const std::vector<fs::path>& inputs,
                                          const std::vector<fs::path>& includes,
                                          const fs::path& output);

namespace impl {
cmd::Ret compile_unit(const Target& target, const fs::path& unit,
                      const std::string& obj);
bool link_exe(Target& info, const std::vector<std::string>& objs);
bool link_lib(Target& info, const std::vector<std::string>& objs);
bool link_dll(Target& info, const std::vector<std::string>& objs);
} // namespace impl
} // namespace csc
