#pragma once

#include "profile.hpp"

struct formattor : step_visitor {
    profile_t *profile;

    void show();

private:
    size_t depth;
    static const char *get_prefix(step::step &step);

public:
    int visit(step::step_group &step);
    int visit(step::confirm &step);
    int visit(step::user_input &step);
    int visit(step::delay &step);
    int visit(step::send &step);
    int visit(step::recv &step);
    int visit(step::recv_str &step);
    int visit(step::math &step);
};
