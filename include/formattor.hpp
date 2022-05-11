#pragma once

#include "profile.hpp"

struct formattor : step_visitor {
    size_t depth;
    std::shared_ptr<profile_t> profile;

private:
    static const char *get_prefix(step::step &step);

public:
    void *visit(step::step_group &step);
    void *visit(step::confirm &step);
    void *visit(step::user_input &step);
    void *visit(step::delay &step);
    void *visit(step::send &step);
    void *visit(step::recv &step);
    void *visit(step::math &step);
};
