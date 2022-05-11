#include "scpi.hpp"
#include "formattor.hpp"
#include "yaml.hpp"

int main() {
    auto str = read_file("profiles/a.yaml");
    auto tree = parse_string(str);

    chnl_map chnls;
    tree["channels"] >> chnls;

    auto profile = std::make_shared<profile_t>();
    tree["steps"] >> profile->steps;

    formattor fmt{};
    fmt.profile = profile;
    fmt.show();
}
