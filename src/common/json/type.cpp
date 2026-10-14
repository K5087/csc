#include <json/type.h>

namespace json {
#define GET(type)                                                     \
    auto ptr = std::get_if<type>(&data);                              \
    if (!ptr) {                                                       \
        throw json_error(std::string("Value contains type is not ") + \
                         typeid(type).name());                        \
    };                                                                \
    return *ptr;

std::string_view Type::get() { return view; }

String::String(const char* str) : Type(str) {}

Value::Value(type&& value) : data(std::forward<type>(value)) {}

const Value& Object::operator[](const String& key) const {
    auto it = pairs.find(key);
    if (it == pairs.end()) {
        throw json_error(std::string("unexist \"") + key.view +
                         "\" in Object.\n" + view);
    }
    return it->second;
}

Array::iterator Array::begin() { return values.begin(); }

Array::iterator Array::end() { return values.end(); }

Array::const_iterator Array::begin() const { return values.begin(); }

Array::const_iterator Array::end() const { return values.end(); }

const Value& Array::operator[](std::size_t index) const {
    if (index >= values.size()) {
        throw json_error("access index" + std::to_string(index) +
                         "out of range in Array.");
    }

    return values[index];
}

std::size_t Array::size() const noexcept { return values.size(); }

const Null& Value::get_null() const { GET(Null); }

const Array& Value::get_array() const { GET(Array); }

const Object& Value::get_object() const { GET(Object); }

const String& Value::get_string() const { GET(String); }

const Number& Value::get_number() const { GET(Number); }

const Boolean& Value::get_boolean() const { GET(Boolean); }

std::optional<const Value&> Object::find(const String& key) const {
    auto it = pairs.find(key);
    if (it == pairs.end()) { return std::nullopt; }
    return it->second;
}
} // namespace json
