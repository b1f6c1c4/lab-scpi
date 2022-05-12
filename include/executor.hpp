#pragma once

#include "profile.hpp"
#include "scpi.hpp"

class executor : public step_visitor {
public:
    chnl_map *_chnls;
    profile_t *_profile;

private:
    size_t _depth;
    step::steps_t *_ns;

    enum {
        IDLE,
        STEP_OVER,
        STEP_OUT,
        RUNNING,
    } _state;
    size_t _limit;

public:
    void reset();
    void start();
    void step_in();
    void step_over();
    void step_out();
    void reverse_step_in();
    void reverse_step_over();
    void reverse_step_out();

private:
    bool next();
    bool exec(step::steps_t &steps);

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
