#pragma once

namespace fancy {

    void init();
    void quit();

    enum kind_t {
        NONE,
        SIGNAL,
        CHAR,
        CTRL_CHAR,
        EOT,
        ESCAPE,
        STRING,
        DOUBLE,
        INVALID, // string contains raw user input
    };
    enum class escape_t {
        NONE,
        INVALID,
        ESC,
        UP,
        DOWN,
        RIGHT,
        LEFT,
        INS,
        DEL,
        HOME,
        END,
        PAGE_UP,
        PAGE_DOWN,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
    };

    struct user_input {
        kind_t kind;
        union {
            int signal;
            char chr;
            escape_t escape;
            // Note: statically allocated, do NOT free()
            const char *string;
            double value;
        };
    };
    user_input check_sig();
    user_input event_loop();
    user_input request_string();
    user_input request_double();

}
