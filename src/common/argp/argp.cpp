#include <argp/argp.h>
#include <assert.h>
#include <iomanip>
#include <iostream>

namespace argp {

void PosOption::handle_rule(Parser& parser, int& index, int argc, char** argv) {
    switch (rule.boundary) {
        case Boundary::one_arg: args.push_back(argv[index]); break;
        case Boundary::another_rule:
            args.push_back(argv[index]);
            while (index + 1 < argc && !parser.is_opt(argv[index + 1])) {
                index++;
                args.push_back(argv[index]);
            }
            break;
        default:
            throw std::runtime_error("PosPosition don't support this Boundary");
    }
}

void Parser::parse(int argc, char** argv) {
    size_t pos = 0;
    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
        if (!is_opt(arg)) {
            if (pos < posoptions.size()) {
                posoptions[pos].handle_rule(*this, i, argc, argv);
                pos++;
            }
            continue;
        }
        Option& option = options[index[arg]];
        switch (option.rule.boundary) {
            case Boundary::get_self: option.args.push_back(arg); break;
            case Boundary::one_arg:
                if (i + 1 >= argc && is_opt(argv[i + 1])) {
                    throw std::invalid_argument(std::string(arg) +
                                                "need one param!");
                }
                option.args.push_back(argv[i + 1]);
                i++;
                break;
            case Boundary::another_rule:
                while (i + 1 < argc && !is_opt(argv[i + 1])) {
                    option.args.push_back(argv[i + 1]);
                    i++;
                }
                break;
        }
    }
}

void Parser::add_opt(std::vector<std::string_view> keys, std::string_view help,
                     Boundary boundary) {
    Option option;
    option.keys = keys;

    option.help = help;
    option.rule = Rule(boundary);
    for (auto i : option.keys) {
        assert(index.count(i) == 0);
        index.emplace(i, options.size());
    }

    options.push_back(std::move(option));
}

void Parser::add_pos(std::string_view name, bool required,
                     std::string_view help, Boundary boundary) {
    PosOption option;
    option.name = name;
    option.help = help;
    option.rule = Rule(boundary);
    option.required = required;
    posoptions.push_back(std::move(option));
}

std::string_view Parser::get_arg(std::string_view key,
                                 std::string_view default_value) {
    assert(index.count(key) != 0);
    if (options[index[key]].args.empty()) {
        return default_value;
    } else {
        return options[index[key]].args[0];
    }
}

const std::vector<std::string_view>& Parser::get_args(std::string_view key) {
    assert(index.count(key) != 0);
    return options[index[key]].args;
}

const std::vector<std::string_view>& Parser::get_pos(size_t pos) {
    return posoptions[pos].args;
}

bool Parser::is_opt(std::string_view arg) { return index.count(arg) != 0; }

void Parser::print_helper(std::string_view name) {
    std::cout << "Usage: " << name;

    // 打印位置参数占位符
    for (const auto& option : posoptions) {
        std::cout << " <" << option.name << ">";
    }
    std::cout << " [options]\n\n";

    // 打印位置参数列表
    if (!posoptions.empty()) {
        std::cout << "Positional arguments:\n";
        for (const auto& pd : posoptions) {
            std::string label = "<" + pd.name + ">";
            std::cout << "  " << std::left << std::setw(24) << label << pd.help;
            if (!pd.required) { std::cout << " [optional]"; }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    // 打印命名参数列表
    std::cout << "Options:\n";
    for (const auto& option : options) {
        std::string desc = std::string(option.keys[0]);
        for (std::size_t i = 1; i < option.keys.size(); i++) {
            desc += ", " + std::string(option.keys[i]);
        }
        std::cout << "  " << std::left << std::setw(24) << desc << option.help
                  << '\n';
    }
}

} // namespace argp
