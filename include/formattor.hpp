#pragma once

#include "profile.hpp"

class formattor : public step_visitor {
    profile_t *_profile;
    size_t _depth;
    static const char *get_prefix(step::step &step);

public:

    explicit formattor(profile_t *profile);
    void show();

    int visit(step::step_group &step);
    int visit(step::confirm &step);
    int visit(step::user_input &step);
    int visit(step::delay &step);
    int visit(step::send &step);
    int visit(step::recv &step);
    int visit(step::recv_str &step);
    int visit(step::math &step);
};
