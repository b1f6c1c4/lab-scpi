#include "executor.hpp"
#include "formattor.hpp"
#include "scpi.hpp"
#include "yaml.hpp"
#include "fancy_cin.hpp"

#include <fstream>

void show_help() {
    std::cout << R"(==== Help ====
NORMAL mode:
Ctrl-D: Quit and save
Ctrl-Q: Quit and discard
Ctrl-\: Quit immediately
Ctrl-C: Interrupt
r/Home: Reverse everything
c/Enter: Start/Continue execution
s/Right: Step-in
n/Down: Step-over
f/End: Step-out
Backscape: Reverse step-in
Up: Reverse step-over
Left: Reverse step-out
Space: Refresh display

INSERT mode:
Ctrl-D: Abort this step
Ctrl-C: Abort this step
Enter: Save and proceed
Ctrl-Q: Quit program immediately
)" << std::flush;
}

enum class normal {
    QUIT_SAVE,
    QUIT_NO_SAVE,
    REFRESH,
    RUN,
};

normal process_normal_mode(executor &exc) {
    std::cout << "NORMAL mode, command requested" << std::endl;
input_again:
    switch (auto ui = fancy::event_loop(); ui.kind) {
        case fancy::SIGNAL:
            std::cout << "To quit & save, press  Ctrl-D" << std::endl;
            goto input_again;
        case fancy::CTRL_CHAR:
            switch (ui.chr) {
                case 'D':
                    return normal::QUIT_SAVE;
                case 'Q':
                    return normal::QUIT_NO_SAVE;
                case 'M': // Enter
                    exc.start();
                    return normal::RUN;
                case 'H': // Backscape
                    exc.reverse_step_in();
                    return normal::REFRESH;
                default:
                    std::cout << "Unexpected command Ctrl-" << ui.chr << std::endl;
                    goto input_again;
            }
        case fancy::CHAR:
            switch (ui.chr) {
                case 'r':
                    exc.reset();
                    return normal::REFRESH;
                case 'c':
                    exc.start();
                    return normal::RUN;
                case 's':
                    exc.step_in();
                    return normal::RUN;
                case 'n':
                    exc.step_over();
                    return normal::RUN;
                case 'f':
                    exc.step_out();
                    return normal::RUN;
                case ' ':
                    return normal::REFRESH;
                case 'h':
                case '?':
                    show_help();
                    goto input_again;
                default:
                    std::cout << "Unexpected command " << ui.chr << std::endl;
                    goto input_again;
            }
        case fancy::ESCAPE:
            switch (ui.escape) {
                case fancy::escape_t::LEFT:
                    exc.reverse_step_out();
                    return normal::REFRESH;
                case fancy::escape_t::RIGHT:
                    exc.step_in();
                    return normal::RUN;
                case fancy::escape_t::UP:
                    exc.reverse_step_over();
                    return normal::REFRESH;
                case fancy::escape_t::DOWN:
                    exc.step_over();
                    return normal::RUN;
                case fancy::escape_t::HOME:
                    exc.reset();
                    return normal::REFRESH;
                case fancy::escape_t::END:
                    exc.step_out();
                    return normal::RUN;
                case fancy::escape_t::F1:
                    show_help();
                    goto input_again;
                default:
                    std::cout << "Unexpected escape command " << (int)ui.escape << std::endl;
                    goto input_again;
            }
        case fancy::EOT:
            return normal::QUIT_SAVE;
        default:
            std::cout << "Unexpected kind " << (int)ui.kind << std::endl;
            goto input_again;
    }
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

    formattor fmt{ &profile };
    executor exc{ &chnls, &profile };

    fancy::init();

    fmt.show();
    while (true) {
        switch (process_normal_mode(exc)) {
            case normal::QUIT_SAVE:
                if (std::ofstream fout{ argv[2] }; fout.good()) {
                    fout << profile;
                } else {
                    std::cout << "Warning: Cannot open the file " << argv[2] << " for writing, writing to stdout instead\n\n";
                    std::cout << profile;
                }
            case normal::QUIT_NO_SAVE:
                return 0;
            case normal::RUN:
                while (exc.run()) {
                    if (auto ui = fancy::check_sig(); ui.kind) {
                        std::cout << "Interrupted" << std::endl;
                        break;
                    }
                    fmt.show();
                }
            case normal::REFRESH:
                fmt.show();
                break;
        }
    }

}
