#pragma once

#include "profile.hpp"
#include "scpi.hpp"

struct executor : step_visitor {
    std::shared_ptr<chnl_map> chnls;
    std::shared_ptr<profile_t> profile;
    size_t depth;
    step::steps_t *ns;

    bool next();
    bool exec(step::steps_t &steps);

    void *visit(step::step_group &step);
    void *visit(step::confirm &step);
    void *visit(step::user_input &step);
    void *visit(step::delay &step);
    void *visit(step::send &step);
    void *visit(step::recv &step);
    void *visit(step::recv_str &step);
    void *visit(step::math &step);
};
