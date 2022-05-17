#include "fancy_cin.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cttrie/cttrie.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

using namespace std::string_literals;

static volatile std::sig_atomic_t g_int;

static bool g_is_enabled{}, g_is_raw;
static termios g_original{};
static termios g_raw_mode;

static void sig_handler(int sig) {
    if (g_int || sig == SIGQUIT || sig == SIGTERM)
        std::quick_exit(128 + sig);
    g_int = sig;
}

namespace fancy {

    void init() {
        if (g_is_enabled) return;
        if (!isatty(STDIN_FILENO)) return;
        if (tcgetattr(STDIN_FILENO, &g_original) < 0) return;

        g_int = 0;

        struct sigaction act;
        act.sa_handler = &sig_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        if (sigaction(SIGINT, &act, nullptr) ||
                sigaction(SIGQUIT, &act, nullptr)) {
            throw std::runtime_error{ "Cannot install sigaction: "s + std::strerror(errno) };
        }

        g_raw_mode = g_original;
        g_raw_mode.c_iflag &= ~(ICRNL | ISTRIP | IXON);
        // g_raw_mode.c_oflag &= ~(OPOST);
        g_raw_mode.c_cflag |= (CS8);
        g_raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        g_raw_mode.c_cc[VMIN] = 0; g_raw_mode.c_cc[VTIME] = 1;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_enabled = true, g_is_raw = true;
        std::at_quick_exit(&quit);
        std::atexit(&quit);
    }

    void quit() {
        if (g_is_enabled) {
            std::signal(SIGINT, SIG_DFL);
            std::signal(SIGQUIT, SIG_DFL);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original);
            g_is_enabled = false;
        }
    }

    static void make_canon() {
        if (!g_is_enabled)
            throw std::runtime_error{ "Not fancy::init() 'ed" };
        if (!g_is_raw) return;
        g_raw_mode.c_iflag |= (ICRNL);
        g_raw_mode.c_lflag |= (ECHO | ICANON | IEXTEN);
        g_raw_mode.c_cc[VEOF] = VINTR;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_raw = false;
    }

    static void make_raw() {
        if (!g_is_enabled)
            throw std::runtime_error{ "Not fancy::init() 'ed" };
        if (g_is_raw) return;
        g_raw_mode.c_iflag &= ~(ICRNL);
        g_raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        g_raw_mode.c_cc[VEOF] = VEOF;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_raw_mode) < 0) return;
        g_is_raw = true;
    }

    user_input check_sig() {
        user_input ui;
        if (auto v = g_int) {
            ui.kind = SIGNAL;
            ui.signal = v;
            g_int = 0;
            return ui;
        }
        ui.kind = NONE;
        return ui;
    }

    user_input event_loop() {
        make_raw();
        user_input ui;
again:
        auto len = read(STDIN_FILENO, &ui.chr, 1);
        if (auto v = g_int) {
            ui.kind = SIGNAL;
            ui.signal = v;
            g_int = 0;
            return ui;
        }
        switch (len) {
            case 0:
                goto again;
            case -1:
                switch (errno) {
                    case EAGAIN:
                    case EINTR:
                        goto again;
                    default:
                        throw std::runtime_error{ "read(2): "s + std::strerror(errno) };
                }
            default:
                if (ui.chr >= 0x20) {
                    ui.kind = CHAR;
                    return ui;
                }
                if (ui.chr != '\e') {
                    ui.kind = CTRL_CHAR;
                    ui.chr ^= 0x40;
                    return ui;
                }
                ui.kind = ESCAPE;
                char buf[5];
                len = read(STDIN_FILENO, &buf, 4);
                if (auto v = g_int) {
                    ui.kind = SIGNAL;
                    ui.signal = v;
                    g_int = 0;
                    return ui;
                }
                switch (len) {
                    case 0:
                        ui.escape = escape_t::ESC;
                        return ui;
                    case -1:
                        switch (errno) {
                            case EAGAIN:
                            case EINTR:
                                ui.escape = escape_t::ESC;
                                return ui;
                            default:
                                throw std::runtime_error{ "read(2): "s + std::strerror(errno) };
                        }
                    default:
                        buf[len] = '\0';
                        TRIE((const char *)buf) ui.escape = escape_t::INVALID;
                        CASE("[A") ui.escape = escape_t::UP;
                        CASE("[B") ui.escape = escape_t::DOWN;
                        CASE("[C") ui.escape = escape_t::RIGHT;
                        CASE("[D") ui.escape = escape_t::LEFT;
                        CASE("[H") ui.escape = escape_t::HOME;
                        CASE("[F") ui.escape = escape_t::END;
                        CASE("[1~") ui.escape = escape_t::HOME;
                        CASE("[2~") ui.escape = escape_t::INS;
                        CASE("[3~") ui.escape = escape_t::DEL;
                        CASE("[4~") ui.escape = escape_t::END;
                        CASE("[5~") ui.escape = escape_t::PAGE_UP;
                        CASE("[6~") ui.escape = escape_t::PAGE_DOWN;
                        CASE("[7~") ui.escape = escape_t::HOME;
                        CASE("[8~") ui.escape = escape_t::INS;
                        CASE("OH") ui.escape = escape_t::HOME;
                        CASE("OF") ui.escape = escape_t::END;
                        CASE("OP") ui.escape = escape_t::F1;
                        CASE("OQ") ui.escape = escape_t::F2;
                        CASE("OR") ui.escape = escape_t::F3;
                        CASE("OS") ui.escape = escape_t::F4;
                        CASE("[15~") ui.escape = escape_t::F5;
                        CASE("[17~") ui.escape = escape_t::F6;
                        CASE("[18~") ui.escape = escape_t::F7;
                        CASE("[19~") ui.escape = escape_t::F8;
                        CASE("[20~") ui.escape = escape_t::F9;
                        CASE("[21~") ui.escape = escape_t::F10;
                        CASE("[23~") ui.escape = escape_t::F11;
                        CASE("[24~") ui.escape = escape_t::F12;
                        ENDTRIE;
                        return ui;
                }
        }
    }

    static user_input request_line() {
        make_canon();
        user_input ui;
        static char buf[4096];
again:
        auto len = read(STDIN_FILENO, buf, 4096);
        if (auto v = g_int) {
            ui.kind = SIGNAL;
            ui.signal = v;
            g_int = 0;
            make_raw();
            return ui;
        }
        switch (len) {
            case 0:
                ui.kind = EOT;
                make_raw();
                return ui;
            case -1:
                switch (errno) {
                    case EAGAIN:
                    case EINTR:
                        goto again;
                    default:
                        make_raw();
                        throw std::runtime_error{ "read(2): "s + std::strerror(errno) };
                }
            default:
                ui.kind = STRING;
                buf[len] = '\0';
                ui.string = buf;
                make_raw();
                return ui;
        }
    }

    user_input request_string() {
        std::cout << "(INSERT mode, string) > " << std::flush;
        return request_line();
    }

    user_input request_double() {
        std::cout << "(INSERT mode, number) > " << std::flush;
        auto ui = request_line();
        if (ui.kind != STRING)
            return ui;
        char *end;
        if (auto val = std::strtod(ui.string, &end); *end != '\n' && *end != '\0') {
            ui.kind = INVALID;
        } else {
            ui.kind = DOUBLE;
            ui.value = val;
        }
        return ui;
    }

}
