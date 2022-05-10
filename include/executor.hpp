#include "profile.hpp"
#include "scpi.hpp"

namespace c4::yml {
    class NodeRef;
}

class executor : public step_visitor {
    std::shared_ptr<chnl_map> _chnls;
    step::steps_t _steps;

public:
    executor(std::shared_ptr<chnl_map> ch, const c4::yml::NodeRef &n);

public:
    void *visit(step::step_group &step);
    void *visit(step::confirm &step);
    void *visit(step::user_input &step);
    void *visit(step::delay &step);
    void *visit(step::send &step);
    void *visit(step::recv &step);
    void *visit(step::math &step);
};
