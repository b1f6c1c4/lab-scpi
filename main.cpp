#include "executor.hpp"
#include "formattor.hpp"
#include "scpi.hpp"
#include "yaml.hpp"
#include "fancy_cin.hpp"

#include <csignal>
#include <fstream>

void sigint_handler(int sig) {
    std::cout << ">SIGINT<" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: lab <profile.yaml> <data.json>\n";
        return 1;
    }

    auto str = read_file(argv[1]);
    auto tree = parse_string(str);
    tree.resolve();

    chnl_map chnls;
    tree["channels"] >> chnls;

    profile_t profile;
    tree["steps"] >> profile.steps;

    if (std::ifstream fin{ argv[2] }; fin.good()) {
        fin >> profile;
    } else {
        std::cout << "Warning: Cannot open the file " << argv[2] << " for reading\n";
        std::cout << "\e[1m\e[35mPlease indicate the name of the new profile:\e[0m" << std::endl;
        std::getline(std::cin, profile.name);
    }

    fancy::init();
    formattor fmt{};
    executor exc{};
    fmt.profile = &profile;
    exc.chnls = &chnls;
    exc.profile = &profile;

    std::signal(SIGINT, &sigint_handler);
    do {
        fmt.show();
    } while (exc.next());
    fmt.show();
    sleep(1);

    if (std::ofstream fout{ argv[2] }; fout.good()) {
        fout << profile;
    } else {
        std::cout << "Warning: Cannot open the file " << argv[2] << " for writing, writing to stdout instead\n\n";
        std::cout << profile;
    }
}
