#pragma once
#include <map>
#include <string_view>
#include <typeinfo>
#include <variant>
#include <vector>

namespace json {

struct Value;

class json_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Type {
    Type(std::string_view view);
    std::string_view get();
    std::string_view view;
};

struct String : public Type {
    using Type::Type;
    String(const char* str);

    auto operator<=>(const String& other) const { return view <=> other.view; }
};

struct Number : public Type {};

struct Boolean : public Type {};

struct Null : public Type {};

struct Object : public Type {
    std::map<String, Value> pairs;

    const Value& operator[](const String& key) const;
    std::optional<const Value&> find(const String& key) const;
};

struct Array : public Type {
    std::vector<Value> values;

public:
    using iterator = std::vector<Value>::iterator;
    using const_iterator = std::vector<Value>::const_iterator;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;
    std::size_t size() const noexcept;
    const Value& operator[](std::size_t index) const;
};

class Value {
public:
    using type = std::variant<Boolean, Null, Object, Array, Number, String>;
    Value(type&& value);

    template <typename T>
        requires(!std::same_as<std::remove_cvref_t<T>, Value> &&
                 std::is_constructible_v<type, T&&>)
    Value(T&& value) : data(std::forward<T>(value)) {}

public:
    template <typename T>
    T get() const {
        auto ptr = std::get_if<T>(&data);
        if (!ptr) {
            throw json_error(std::string("Value contains type is not ") +
                             typeid(T).name());
        };
        return *ptr;
    }

    template <typename T>
    std::string_view get_view() const {
        auto ptr = std::get_if<T>(&data);
        if (!ptr) {
            throw json_error(std::string("Value contains type is not ") +
                             typeid(T).name());
        };
        return *ptr->view;
    }

    std::string_view get_view() const noexcept {
        return std::visit([](auto& v) { return v.view; }, data);
    }

    const Null& get_null() const;
    const Array& get_array() const;
    const Object& get_object() const;
    const String& get_string() const;
    const Number& get_number() const;
    const Boolean& get_boolean() const;

    friend void print_json(const Value& value, unsigned int indent);

private:
    type data;
};

enum Grammar {
    ws,
    value,
    begin_array,
    begin_objct,
    end_array,
    end_object,
    name_separator,
    value_separator
};

} // namespace json
