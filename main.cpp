#include "executor.hpp"
#include "formattor.hpp"
#include "scpi.hpp"
#include "yaml.hpp"
#include "fancy_cin.hpp"

#include <fstream>

void show_help() {
    std::cout << R"(Help
Ctrl-D: Quit and save
Ctrl-C: Interrupt
r: Restart everything
c: Start/Continue execution
Enter: Step-in
s: Step-in
n: Step-over
f: Step-out
)" << std::flush;
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

    formattor fmt{};
    executor exc{};
    fmt.profile = &profile;
    exc.chnls = &chnls;
    exc.profile = &profile;

    fancy::init();
    while (true) {
        fmt.show();
        std::cout << "NORMAL mode, command requested" << std::endl;
again:
        auto ui = fancy::event_loop();
        switch (ui.kind) {
            case fancy::SIGNAL:
                std::cout << "To quit, press  Ctrl-D" << std::endl;
                goto again;
            case fancy::CTRL_CHAR:
                switch (ui.chr) {
                    case 'D':
                        goto quitting;
                    case 'M':
                        exc.next();
                        continue;
                    default:
                        std::cout << "Unexpected command Ctrl-" << ui.chr << std::endl;
                        goto again;
                }
            case fancy::CHAR:
                switch (ui.chr) {
                    case 's':
                    case 'n':
                    case 'f':
                        exc.next();
                        continue;
                    case 'h':
                    case '?':
                        show_help();
                        goto again;
                    default:
                        std::cout << "Unexpected command " << ui.chr << std::endl;
                        goto again;
                }
            case fancy::ESCAPE:
                switch (ui.escape) {
                    case fancy::escape_t::LEFT:
                        std::cout << "<-" << std::endl;
                        goto again;
                    case fancy::escape_t::RIGHT:
                        std::cout << "->" << std::endl;
                        goto again;
                    case fancy::escape_t::UP:
                        std::cout << "^^" << std::endl;
                        goto again;
                    case fancy::escape_t::DOWN:
                        std::cout << "vv" << std::endl;
                        goto again;
                    case fancy::escape_t::F1:
                        show_help();
                        goto again;
                    default:
                        std::cout << "Unexpected escape command " << (int)ui.escape << std::endl;
                        goto again;
                }
            case fancy::EOT:
                goto quitting;
        }
    }
quitting:

    if (std::ofstream fout{ argv[2] }; fout.good()) {
        fout << profile;
    } else {
        std::cout << "Warning: Cannot open the file " << argv[2] << " for writing, writing to stdout instead\n\n";
        std::cout << profile;
    }

    fancy::quit();
}
