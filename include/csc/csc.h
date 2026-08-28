#pragma once
#include <cmd/cmd.h>
#include <log/log.h>

#include <filesystem>
#include <functional>
#include <vector>

namespace csc {
class Target;
namespace fs = std::filesystem;
using Path = std::filesystem::path;

enum class UpdateStatus { success, noneed, failed };

/**
 * @brief use input includes and output make compile command(default add c++26
 * flag)
 *
 * @param inputs cpp sources
 * @param includes include dir
 * @param output output file
 */
std::vector<std::string> make_compile_cmd(const std::vector<fs::path>& inputs,
                                          const std::vector<fs::path>& includes,
                                          const fs::path& output);
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
 * @brief Check whether a file is out of date.
 *
 * @param file The file to check.(should be latest)
 * @param inputs Files whose modification times are compared against @p file.
 * @return true if @p file is out of date; false otherwise.
 */
bool is_outdated(const std::filesystem::path& file,
                 const std::vector<fs::path>& inputs);

bool build_target(std::shared_ptr<Target> target,
                  std::function<void()> before_build = {});

namespace impl {
// parse .d file,get obj dep
std::vector<fs::path> get_deps(const fs::path& obj);
cmd::Ret compile_unit(const Target& target, const fs::path& unit,
                      const std::string& obj);
bool link_exe(Target& info, const std::vector<std::string>& objs);
bool link_lib(Target& info, const std::vector<std::string>& objs);
bool link_dll(Target& info, const std::vector<std::string>& objs);
} // namespace impl
} // namespace csc
