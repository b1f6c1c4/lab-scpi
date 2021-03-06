#pragma once

#include <memory>
#include <deque>
#include <string>
#include <variant>
#include <vector>

struct profile;

namespace step {
    struct step;
    struct step_group;
    struct confirm;
    struct user_input;
    struct delay;
    struct send;
    struct recv;
    struct recv_str;
    struct math;
}

struct step_visitor {
    virtual int visit(step::step_group &step) = 0;
    virtual int visit(step::confirm &step) = 0;
    virtual int visit(step::user_input &step) = 0;
    virtual int visit(step::delay &step) = 0;
    virtual int visit(step::send &step) = 0;
    virtual int visit(step::recv &step) = 0;
    virtual int visit(step::recv_str &step) = 0;
    virtual int visit(step::math &step) = 0;
};

namespace step {

    struct steps_t : std::vector<std::unique_ptr<step>> {
        steps_t *parent{};
    };

    using ref_t = std::vector<std::variant<std::monostate, size_t, std::string>>;

    struct step {
        std::string id;
        std::string name;
        enum {
            QUEUED,
            CURRENT,
            FINISHED,
            REVERTED,
        } status;

        virtual int accept(step_visitor &sv) = 0;
    };

    struct valued_step : step {
        double value;
        std::string unit;
    };

    struct step_group : step {
        steps_t steps;
        ref_t digest;
        bool vertical;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct confirm : step {
        std::string prompt;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct user_input : valued_step {
        std::string prompt;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct delay : step {
        int seconds;
        int nanoseconds;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct send : step {
        std::string channel;
        std::string cmd;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct recv : valued_step {
        std::string channel;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct recv_str : step {
        std::string channel;
        std::string value;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct math : valued_step {
        struct operand {
            enum {
                DOUBLE,
                REFERENCE,
            } kind;
            double value;
            ref_t index;
            double operator()(steps_t &steps);
        };

        enum {
            ADD,
            SUB,
            MUL,
            DIV,
        } op;
        std::vector<operand> operands;

        int accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    step *get(steps_t &steps, const ref_t &cont);
    void resolve_parent(steps_t &steps);
}

struct profile_t {
    std::string name;
    step::steps_t steps;
    std::deque<size_t> current;

    step::step *operator()();
};

std::istream &operator>>(std::istream &is, profile_t &cpp);
std::ostream &operator<<(std::ostream &os, const profile_t &cpp);

namespace c4::yml {
    class NodeRef;
    bool read(const NodeRef &n, std::unique_ptr<step::step> *obj);
    bool read(const NodeRef &n, step::steps_t *obj);
    bool read(const NodeRef &n, step::step_group *obj);
    bool read(const NodeRef &n, step::confirm *obj);
    bool read(const NodeRef &n, step::user_input *obj);
    bool read(const NodeRef &n, step::delay *obj);
    bool read(const NodeRef &n, step::send *obj);
    bool read(const NodeRef &n, step::recv *obj);
    bool read(const NodeRef &n, step::recv_str *obj);
    bool read(const NodeRef &n, step::math *obj);
    bool read(const NodeRef &n, step::math::operand *obj);
}
