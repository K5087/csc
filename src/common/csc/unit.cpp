#include <csc/unit.h>

namespace csc {
Unit::Unit(const fs::path& Path, const fs::path& build_path)
    : path(Path),
      obj(fs::path(build_path).replace_extension(".o")),
      pcm(fs::path(build_path).replace_extension(".pcm")),
      depend(fs::path(build_path).replace_extension(".d")),
      p1689(fs::path(build_path).replace_extension(".json")),
      path_string(path.generic_string()),
      obj_path(obj.generic_string()),
      pcm_path(pcm.generic_string()),
      depend_path(depend.generic_string()),
      p1689_path(p1689.generic_string()) {}
} // namespace csc
