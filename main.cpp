#include "executor.hpp"
#include "formattor.hpp"
#include "scpi.hpp"
#include "yaml.hpp"
#include "fancy_cin.hpp"

#include <fstream>

void show_help() {
    std::cout << R"(==== Help ====
Ctrl-D: Quit and save
Ctrl-Q: Quit and discard
Ctrl-C: Interrupt
r/Home: Reverse everything
c/Enter: Start/Continue execution
s/Right: Step-in
n/Down: Step-over
f/End: Step-out
Backscape: Reverse step-in
Up: Reverse step-over
Left: Reverse step-out
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
    exc._chnls = &chnls;
    exc._profile = &profile;

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
                    case 'Q':
                        return 0;
                    case 'M': // Enter
                        exc.start();
                        continue;
                    case 'H': // Backscape
                        exc.reverse_step_in();
                        continue;
                    default:
                        std::cout << "Unexpected command Ctrl-" << ui.chr << std::endl;
                        goto again;
                }
            case fancy::CHAR:
                switch (ui.chr) {
                    case 'r':
                        exc.reset();
                        continue;
                    case 'c':
                        exc.start();
                        continue;
                    case 's':
                        exc.step_in();
                        continue;
                    case 'n':
                        exc.step_over();
                        continue;
                    case 'f':
                        exc.step_out();
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
                        exc.reverse_step_out();
                        continue;
                    case fancy::escape_t::RIGHT:
                        exc.step_in();
                        continue;
                    case fancy::escape_t::UP:
                        exc.reverse_step_over();
                        continue;
                    case fancy::escape_t::DOWN:
                        exc.step_over();
                        continue;
                    case fancy::escape_t::HOME:
                        exc.reset();
                        continue;
                    case fancy::escape_t::END:
                        exc.step_out();
                        continue;
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
}
