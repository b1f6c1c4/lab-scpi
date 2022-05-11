#define RYML_SINGLE_HDR_DEFINE_NOW
#include <rapidyaml.hpp>

std::string operator+(const char *s, c4::csubstr buf) {
    auto len = std::strlen(s);
    std::string ans;
    ans.resize(len + buf.len);
    memcpy(&ans[0], s, len);
    memcpy(&ans[len], buf.str, buf.len);
    return ans;
}

std::string read_file(const char *filename) {
    std::string v;
    auto fp = std::fopen(filename, "rb");
    std::fseek(fp, 0, SEEK_END);
    auto sz = std::ftell(fp);
    v.resize(static_cast<std::string::size_type>(sz));
    if (!sz) {
        std::fclose(fp);
        return {};
    }
    std::rewind(fp);
    std::fread(&v[0], 1, v.size(), fp);
    std::fclose(fp);
    return v;
};

ryml::Tree parse_string(std::string &contents) {
    return ryml::parse_in_arena(ryml::to_csubstr(contents));
}
