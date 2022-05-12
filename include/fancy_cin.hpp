#pragma once

namespace fancy {

    void init();
    void quit();

    struct user_input {
        enum {
            SIGNAL,
            CONTROL,
            CTRL_D,
            STRING,
            DOUBLE,
            INVALID, // string contains raw user input
        } kind;
        union {
            int signal;
            char control;
            // Note: statically allocated, do NOT free()
            const char *string;
            double value;
        };
    };
    user_input event_loop();
    user_input request_string();
    user_input request_double();

}
