#include <memory>
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
    struct math;
}

struct step_visitor {
    virtual void *visit(step::step_group &step) = 0;
    virtual void *visit(step::confirm &step) = 0;
    virtual void *visit(step::user_input &step) = 0;
    virtual void *visit(step::delay &step) = 0;
    virtual void *visit(step::send &step) = 0;
    virtual void *visit(step::recv &step) = 0;
    virtual void *visit(step::math &step) = 0;
};

namespace step {

    using steps_t = std::vector<std::unique_ptr<step>>;

    struct step {
        std::string name;

        virtual void *accept(step_visitor &sv) = 0;
    };

    struct valued_step : step {
        double value;
    };

    struct step_group : step {
        steps_t steps;

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

}

namespace c4::yml {
    class NodeRef;
    bool read(const NodeRef &n, std::unique_ptr<step::step> *obj);
    bool read(const NodeRef &n, step::step_group *obj);
    bool read(const NodeRef &n, step::confirm *obj);
    bool read(const NodeRef &n, step::user_input *obj);
    bool read(const NodeRef &n, step::delay *obj);
    bool read(const NodeRef &n, step::send *obj);
    bool read(const NodeRef &n, step::recv *obj);
    bool read(const NodeRef &n, step::math *obj);
    bool read(const NodeRef &n, step::math::operand *obj);
}
