#include "fancy_cin.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

using namespace std::string_literals;

static volatile std::sig_atomic_t g_int;

static bool g_is_enabled{}, g_is_raw;
static termios g_original{};
static termios g_raw_mode;

static void sig_handler(int sig) {
    if (g_int)
        std::quick_exit(128 + sig);
    g_int = sig;
}

namespace fancy {

    void init() {
        if (g_is_enabled) return;
        if (!isatty(STDIN_FILENO)) return;
        if (tcgetattr(STDIN_FILENO, &g_original) < 0) return;

#ifndef NDEBUG
        std::printf("About to enter RAW mode, I'm pid=%d\n", getpid());
#endif
        g_int = 0;

        std::signal(SIGINT, &sig_handler);
        std::signal(SIGTERM, &sig_handler);
        std::signal(SIGQUIT, &sig_handler);

        g_raw_mode = g_original;
        g_raw_mode.c_iflag &= ~(ISTRIP | IXON);
        // g_raw_mode.c_oflag &= ~(OPOST);
        g_raw_mode.c_cflag |= (CS8);
        g_raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        g_raw_mode.c_cc[VMIN] = 1; g_raw_mode.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_enabled = true, g_is_raw = true;
        std::at_quick_exit(&quit);
        std::atexit(&quit);
    }

    void quit() {
        if (g_is_enabled) {
#ifndef NDEBUG
            std::printf("About to exit RAW mode, I'm pid=%d\n", getpid());
#endif
            std::signal(SIGINT, SIG_DFL);
            std::signal(SIGTERM, SIG_DFL);
            std::signal(SIGQUIT, SIG_DFL);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original);
            g_is_enabled = false;
        }
    }

    static void make_canon() {
        if (!g_is_enabled)
            throw std::runtime_error{ "Not fancy::init() 'ed" };
        if (!g_is_raw) return;
        g_raw_mode.c_lflag |= (ECHO | ICANON | IEXTEN);
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_raw = false;
    }

    static void make_raw() {
        if (!g_is_enabled)
            throw std::runtime_error{ "Not fancy::init() 'ed" };
        if (g_is_raw) return;
        g_raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_raw = true;
    }

    user_input event_loop() {
        make_raw();
        user_input ui;
        switch (read(STDIN_FILENO, &ui.control, 1)) {
            case 1:
                ui.kind = user_input::CONTROL;
                goto finish;
            case 0:
                ui.kind = user_input::CTRL_D;
                goto finish;
        }
        switch (errno) {
            case EINTR:
                ui.kind = user_input::SIGNAL;
                ui.signal = g_int;
                g_int = 0;
                goto finish;
            default:
                throw std::runtime_error{ "read(2): "s + std::strerror(errno) };
        }
finish:
        return ui;
    }

    user_input request_line() {
        make_canon();
        user_input ui;
        static char buf[4096];
        switch (auto len = read(STDIN_FILENO, buf, 4096)) {
            default:
                ui.kind = user_input::STRING;
                buf[len] = '\0';
                ui.string = buf;
                goto finish;
            case 0:
                ui.kind = user_input::CTRL_D;
                goto finish;
            case -1:
                break;
        }
        switch (errno) {
            case EINTR:
                ui.kind = user_input::SIGNAL;
                ui.signal = g_int;
                g_int = 0;
                goto finish;
            default:
                throw std::runtime_error{ "read(2): "s + std::strerror(errno) };
        }
finish:
        make_raw();
        return ui;
    }

    user_input request_double() {
        auto ui = request_line();
        if (ui.kind == user_input::STRING) {
            char *end;
            auto val = std::strtof(ui.string, &end);
            if (end) {
                ui.kind = user_input::DOUBLE;
                ui.value = val;
            } else {
                ui.kind = user_input::INVALID;
            }
        }
        return ui;
    }

}
