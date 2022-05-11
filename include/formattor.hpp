#pragma once

#include "profile.hpp"

struct formattor : step_visitor {
    std::shared_ptr<profile_t> profile;

    void show();

private:
    size_t depth;
    static const char *get_prefix(step::step &step);

public:
    void *visit(step::step_group &step);
    void *visit(step::confirm &step);
    void *visit(step::user_input &step);
    void *visit(step::delay &step);
    void *visit(step::send &step);
    void *visit(step::recv &step);
    void *visit(step::recv_str &step);
    void *visit(step::math &step);
};
