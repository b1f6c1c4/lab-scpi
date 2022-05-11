#include "executor.hpp"
#include "formattor.hpp"
#include "scpi.hpp"
#include "yaml.hpp"

#include <fstream>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: lab <profile.yaml> <data.json>\n";
        return 1;
    }

    auto str = read_file(argv[1]);
    auto tree = parse_string(str);
    tree.resolve();

    auto chnls = std::make_shared<chnl_map>();
    tree["channels"] >> *chnls;

    auto profile = std::make_shared<profile_t>();
    tree["steps"] >> profile->steps;

    {
        std::ifstream fin{ argv[2] };
        if (fin.good())
            fin >> *profile;
        else {
            std::cerr << "Warning: Cannot open the file " << argv[2] << " for reading\n";
            sleep(1);
        }
    }

    formattor fmt{};
    executor exc{};
    fmt.profile = profile;
    exc.chnls = chnls;
    exc.profile = profile;

    do {
        fmt.show();
    } while (exc.next());
    fmt.show();
    sleep(1);

    {
        std::ofstream fout{ argv[2] };
        fout << *profile;
    }
}
