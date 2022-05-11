#include "fancy_cin.hpp"

#include <csignal>
#include <termios.h>
#include <unistd.h>
#include <cstdio>

static volatile std::sig_atomic_t g_int;

static bool g_is_enabled{};
static termios g_original_settings{};

void fancy::init() {
    if (g_is_enabled) return;
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_original_settings) < 0) return;

    std::printf("About to enter RAW mode, I'm pid=%d\n", getpid());

    // enter raw mode
    termios raw{ g_original_settings };
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 8; /* after 5 bytes or .8 seconds
                                                after first byte seen      */
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; /* immediate - anything       */
    raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0; /* after two bytes, no timer  */
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; /* after a byte or .8 seconds */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) return;
}

void fancy::quit() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
