#include <json/parser.h>

namespace json {
Parser::Parser(std::string_view json) : json(json), index(0) {}

} // namespace json
