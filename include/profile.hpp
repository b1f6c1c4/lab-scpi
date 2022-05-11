#pragma once

#include <memory>
#include <deque>
#include <string>
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
    virtual void *visit(step::step_group &step) = 0;
    virtual void *visit(step::confirm &step) = 0;
    virtual void *visit(step::user_input &step) = 0;
    virtual void *visit(step::delay &step) = 0;
    virtual void *visit(step::send &step) = 0;
    virtual void *visit(step::recv &step) = 0;
    virtual void *visit(step::recv_str &step) = 0;
    virtual void *visit(step::math &step) = 0;
};

namespace step {

    using steps_t = std::vector<std::unique_ptr<step>>;

    struct step {
        std::string name;
        enum {
            QUEUED,
            CURRENT,
            FINISHED,
            REVERTED,
        } status;

        virtual void *accept(step_visitor &sv) = 0;
    };

    struct valued_step : step {
        double value;
        std::string unit;
    };

    struct step_group : step {
        steps_t steps;
        std::vector<size_t> digest;
        bool vertical;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct confirm : step {
        std::string prompt;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct user_input : valued_step {
        std::string prompt;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct delay : step {
        int seconds;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct send : step {
        std::string channel;
        std::string cmd;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct recv : valued_step {
        std::string channel;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct recv_str : step {
        std::string channel;
        std::string value;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    struct math : valued_step {
        struct operand {
            enum {
                DOUBLE,
                REFERENCE,
            } kind;
            double value;
            std::vector<size_t> index;
            double operator()(steps_t &steps);
        };

        enum {
            ADD,
            SUB,
            MUL,
            DIV,
        } op;
        std::vector<operand> operands;

        void *accept(step_visitor &sv) override { return sv.visit(*this); }
    };

    template <typename TContainer>
    auto get(steps_t &steps, TContainer &&cont) {
        step *last{};
        for (auto ptr = &steps; auto &&id : cont) {
            last = ptr->at(id).get();
            if (auto grp = dynamic_cast<step_group *>(last); grp)
                ptr = &grp->steps;
        }
        return last;
    }
}

struct profile_t {
    std::string name;
    step::steps_t steps;
    std::deque<size_t> current;

    step::step &operator()();
};

std::istream &operator>>(std::istream &is, profile_t &cpp);
std::ostream &operator<<(std::ostream &os, const profile_t &cpp);

namespace c4::yml {
    class NodeRef;
    bool read(const NodeRef &n, std::unique_ptr<step::step> *obj);
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
