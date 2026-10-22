#pragma once
#include <cmd/cmd.h>
#include <csc/depend.h>
#include <csc/target.h>
#include <csc/unit.h>

#include <set>
#include <unordered_map>

namespace csc {
enum class Status { bmi, depend, object, link, end };

class Task {
public:
    Task(std::size_t index);

public:
    Status status = Status::bmi;
    cmd::Ret ret;
    // unit index
    std::size_t index;
};

class Graph {
    template <typename T>
    struct RefHash {
        std::size_t operator()(std::reference_wrapper<T> ref) const {
            return std::hash<std::remove_cv_t<T>>{}(ref.get());
        }
    };

    template <typename Key, typename Tp>
    using ref_map =
        std::unordered_map<std::reference_wrapper<Key>, Tp, RefHash<Key>>;

public:
    bool need_rebuild = false;
    Target& target;
    std::vector<Unit> units;

    std::vector<cmd::Ret> rets;
    std::vector<std::shared_ptr<Task>> tasks;

public:
    Graph(Target& target);
    bool parse_p1689(Unit& unit);
    bool build();

    void handle_task(std::shared_ptr<Task> task);
    cmd::Ret gen_p1689(const Unit& unit);
    cmd::Ret compile_module(const Unit& unit) const;
    void stop_all();

    bool recover_depend();

    bool have_cycle();

    cmd::Ret gen_bmi(const Unit& unit) const;

    bool is_newest(std::size_t index) const;

private:
    // this is cache map
    // source_path, module unit index
    ref_map<const fs::path, std::size_t> source_map;
    // logical_name, module unit index
    ref_map<const std::string, std::size_t> logical_map;

    // module provide unit index, require module task
    std::unordered_map<std::size_t, std::set<std::shared_ptr<Task>>>
        module_dep_map;
    // rets index, task index
    std::unordered_map<std::size_t,std::size_t> ret_map;
};

} // namespace csc
