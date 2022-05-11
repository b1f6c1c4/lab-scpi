#pragma once

#include <rapidyaml.hpp>

using namespace std::string_literals;

inline std::string operator+(const char *s, c4::csubstr buf) {
    auto len = std::strlen(s);
    std::string ans;
    ans.resize(len + buf.len);
    memcpy(&ans[0], s, len);
    memcpy(&ans[len], buf.str, buf.len);
    return ans;
}
