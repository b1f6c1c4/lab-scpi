#pragma once

#include <cstdio>
#include <rapidyaml.hpp>

using namespace std::string_literals;

std::string operator+(const char *s, c4::csubstr buf);

std::string read_file(const char *filename);
ryml::Tree parse_string(std::string &contents);
