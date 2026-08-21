#include <csc/csc.h>

namespace rsc {
using Path = std::filesystem::path;
inline const Path script = std::filesystem::current_path() / "scripts";

void init();
} // namespace rsc
