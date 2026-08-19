#include <csc/csc.h>

namespace rsc {
using Path = std::filesystem::path;
inline const Path script = std::filesystem::current_path() / "scripts";

void init();
std::vector<Path> find_file(const std::vector<Path>& path, const std::string& extension);
} // namespace rsc
