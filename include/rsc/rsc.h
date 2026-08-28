#pragma once
#include <csc/csc.h>

namespace rsc {
namespace fs = std::filesystem;
void init();

void install(const std::string& name, const std::string& version = "1.0.0",
             const std::vector<fs::path>& includes = {},
             const std::vector<fs::path>& bins = {},
             const std::vector<fs::path>& static_libs = {},
             const std::vector<fs::path>& shared_libs = {});

void set_current(const std::string& name, const std::string& version);

/* simple get filname (with extension), find in package dir and create symlik*/
void create_shim(const std::string& name,
                 const std::vector<fs::path>& bins = {},
                 const std::vector<fs::path>& shared_libs = {},
                 const std::string& version = "current");
/* simple find file in package dir by string(with extension), create symlink*/
void create_shim(const std::string& name,
                 const std::vector<std::string>& bins = {},
                 const std::vector<std::string>& shared_libs = {},
                 const std::string& version = "current");

namespace get {
/* return rsc install dir */
const fs::path& install_dir();
/* return rsc script dir */
const fs::path& script_dir();
/* return rsc package dir */
const fs::path& package_dir();
/* return rsc bin dir */
const fs::path& bin_dir();
/* return rsc shared lib dir */
const fs::path& lib_dir();

/* return rsc installed package dir */
fs::path package_dir(const std::string& name,
                     const std::string& version = "1.0.0");

} // namespace get

namespace impl {
void create_shim(std::ranges::input_range auto&& names,
                 const fs::path& search_dir, const fs::path symlink_dir);
} // namespace impl
} // namespace rsc
