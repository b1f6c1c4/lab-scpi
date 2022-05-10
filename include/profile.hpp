#include <map>
#include <memory>
#include <string>
#include <vector>

#include "scpi.hpp"

struct profile;

namespace step {

    struct step {
        virtual void execute(profile *pf) = 0;
    };

    struct valued_step : step {
        double value;
    };

    struct confirm : step {
        std::string prompt;

        void execute(profile *pf) override;
    };

    struct delay : step {
        int seconds;

        void execute(profile *pf) override;
    };

    struct send : step {
        std::string channel;
        std::string cmd;

        void execute(profile *pf) override;
    };

    struct recv : valued_step {
        std::string channel;

        void execute(profile *pf) override;
    };

    struct math : valued_step {
        struct operand {
            enum {
                DOUBLE,
                REFERENCE,
            } kind;
            union {
                double value;
                size_t index;
            };
            double operator()(profile *pf) const;
        };

        enum {
            ADD,
            SUB,
            MUL,
            DIV,
        } op;
        std::vector<operand> operands;

        void execute(profile *pf) override;
    };

}

struct profile {
    std::map<std::string, std::unique_ptr<scpi>> chnls;
    std::vector<std::unique_ptr<step::step>> steps;
};
