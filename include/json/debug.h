#pragma once
#include <json/type.h>

namespace json {
void print_json(const Value& value, unsigned int indent);

void print_json(const Null& null, unsigned int indent);
void print_json(const Array& array, unsigned int indent);
void print_json(const Object& object, unsigned int indent);
void print_json(const String& string, unsigned int indent);
void print_json(const Number& number, unsigned int indent);
void print_json(const Boolean& boolean, unsigned int indent);
} // namespace json
