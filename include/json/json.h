#pragma once
#include <log/log.h>

#include <json/encode.h>

#define ENCODE_KEY_VALUE(json, key, value) \
    json.append(key);                      \
    json.append(":");                      \
    encode(json, value);
#define ENCODE_KEY_ARRAY(json, key, array)                 \
    json.append(key);                                      \
    json.append(":[");                                     \
    for (auto& element : array) { encode(json, element); } \
    json.append("]");

namespace json {} // namespace json
