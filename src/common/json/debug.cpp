#include <json/debug.h>

#include <iomanip>
#include <iostream>

namespace json {
void print_indent(unsigned int indent) {
    if (indent) std::cout << std::setw(indent) << ' ';
}

void print_json(const Value& value, unsigned int indent) {
    std::visit([=](const auto& v) { json::print_json(v, indent); }, value.data);
}

void print_json(const Object& object, unsigned int indent) {
    std::cout << "{";
    bool is_first = true;
    for (auto& [key, value] : object.pairs) {
        if (is_first) {
            std::cout << '\n';
            is_first = false;
        } else {
            std::cout << ",\n";
        }
        print_indent(indent + 2);
        print_json(key, indent + 2);
        std::cout << ":";
        print_json(value, indent + 2);
    }
    if (!is_first) {
        std::cout << '\n';
        print_indent(indent);
    }
    std::cout << "}";
}

void print_json(const Null& null, unsigned int indent) {
    std::cout << null.view;
}

void print_json(const Array& array, unsigned int indent) {
    std::cout << "[";
    bool is_first = true;
    for (auto& value : array.values) {
        if (is_first) {
            std::cout << '\n';
            is_first = false;
        } else {
            std::cout << ",\n";
        }
        print_indent(indent + 2);
        print_json(value, indent + 2);
    }
    if (!is_first) {
        std::cout << '\n';
        print_indent(indent);
    }
    std::cout << "]";
}

void print_json(const String& string, unsigned int indent) {
    std::cout << '"' << string.view << '"';
}

void print_json(const Number& number, unsigned int indent) {
    std::cout << number.view;
}

void print_json(const Boolean& boolean, unsigned int indent) {
    std::cout << boolean.view;
}
} // namespace json
