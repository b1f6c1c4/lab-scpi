#pragma once

#include "profile.hpp"
#include "scpi.hpp"

class executor : public step_visitor {
    chnl_map *_chnls;
    profile_t *_profile;
    size_t _depth;
    step::steps_t *_ns;
    size_t _limit; // idle when curr depth < _limit

public:

    executor(chnl_map *chnls, profile_t *profile);

    // shall be followed by  while (run());
    void start();
    void step_in();
    void step_over();
    void step_out();

    void reset();
    void reverse_step_in();
    void reverse_step_over();
    void reverse_step_out();

    bool run();

private:
    bool revt(step::steps_t &steps);
    int exec(step::steps_t &steps);

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
