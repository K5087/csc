#pragma once
#include <csc/target.h>
#include <memory>
#include <vector>

namespace csc {

class Graph {
    std::vector<std::shared_ptr<Target>> build;
};
} // namespace csc
