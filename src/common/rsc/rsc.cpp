#include <rsc/rsc.h>

#include <log/log.h>
#include <rsc/dev.h>
#include <rsc/os.h>

#include <ranges>

namespace rsc {

void init() {
    fs::create_directories(get::script_dir());
    fs::create_directories(get::package_dir());
    fs::create_directories(get::bin_dir());
    fs::create_directories(get::lib_dir());
}

void install(const std::string& name, const std::string& version,
             const std::vector<fs::path>& includes,
             const std::vector<fs::path>& bins,
             const std::vector<fs::path>& static_libs,
             const std::vector<fs::path>& shared_libs) {
    std::string_view ver = version;
    if (!os::valid_name(version)) {
        loge("%s can't be a valid path name,replace to 1.0.0", version.c_str());
        ver = "1.0.0";
    }

    fs::path package_dir = get::package_dir() / name / ver;

    fs::path bin_dir = package_dir / "bin";
    fs::path include_dir = package_dir / "include";
    fs::path static_lib_dir = package_dir / "lib/static";
    fs::path shared_lib_dir = package_dir / "lib/shared";

    auto copy_files =
        [](const std::vector<fs::path>& files, const fs::path& dir,
           fs::copy_options option = fs::copy_options::update_existing) {
            fs::create_directories(dir);
            for (auto& path : files) { fs::copy(path, dir, option); }
        };

    copy_files(bins, bin_dir);
    copy_files(includes, include_dir,
               fs::copy_options::recursive |
                   fs::copy_options::overwrite_existing);
    copy_files(static_libs, static_lib_dir);
    copy_files(shared_libs, shared_lib_dir);
}

void set_current(const std::string& name, const std::string& version) {
    if (!os::valid_name(version)) {
        loge("%s isn't a valid path name", version.c_str());
        return;
    }
    fs::path package = get::package_dir();
    fs::path current = package / name / "current";

    if (fs::is_symlink(current)) { fs::remove(current); }
    fs::create_directory_symlink(package / name / version,
                                 package / name / "current");
}

namespace get {
const fs::path& install_dir() {
    static fs::path install = os::get_home_dir() / "rsc";
    return install;
}

const fs::path& script_dir() {
    static fs::path script = install_dir() / "script";
    return script;
}

const fs::path& package_dir() {
    static fs::path package = install_dir() / "package";
    return package;
}

const fs::path& bin_dir() {
    static fs::path bin = install_dir() / "bin";
    return bin;
}

const fs::path& lib_dir() {
    static fs::path lib = install_dir() / "lib";
    return lib;
}

fs::path package_dir(const std::string& name, const std::string& version) {
    return package_dir() / name / version;
}
} // namespace get

void create_shim(const std::string& name, const std::vector<fs::path>& bins,
                 const std::vector<fs::path>& shared_libs,
                 const std::string& version) {
    fs::path package_bin = get::package_dir(name, version) / "bin";
    fs::path package_shared = get::package_dir(name, version) / "lib/shared";
    fs::path bin_dir = get::bin_dir();
    fs::path lib_dir = get::lib_dir();
    impl::create_shim(bins | std::views::transform([](const fs::path& path) {
                          return path.filename().string();
                      }),
                      package_bin, bin_dir);

    impl::create_shim(shared_libs |
                          std::views::transform([](const fs::path& path) {
                              return path.filename().string();
                          }),
                      package_shared, lib_dir);
}

void create_shim(const std::string& name, const std::vector<std::string>& bins,
                 const std::vector<std::string>& shared_libs,
                 const std::string& version) {
    fs::path package_bin = get::package_dir(name, version) / "bin";
    fs::path package_shared = get::package_dir(name, version) / "lib/shared";
    fs::path bin_dir = get::bin_dir();
    fs::path lib_dir = get::lib_dir();
    impl::create_shim(bins, package_bin, bin_dir);

    impl::create_shim(shared_libs, package_shared, lib_dir);
}

namespace impl {

void create_shim(std::ranges::input_range auto&& names,
                 const fs::path& search_dir, const fs::path symlink_dir) {
    for (auto&& name : names) {
        fs::path bin = search_dir / name;
        if (fs::exists(bin)) {
            fs::path shim = symlink_dir / name;
            if (fs::is_symlink(shim)) { fs::remove(shim); }
            fs::create_symlink(bin, shim);
        } else {
            logw("%s not found in %s", name.c_str(),
                 search_dir.string().c_str());
        }
    }
}

} // namespace impl

} // namespace rsc
