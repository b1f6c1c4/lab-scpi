#include "executor.hpp"

#include <limits>

#include "yaml.hpp"

executor::executor(std::shared_ptr<chnl_map> ch, const c4::yml::NodeRef &n) :
    _chnls{ ch } {
    n["steps"] >> _steps;
}

void *executor::visit(step::step_group &step) {
    std::cerr << "Executing grouped step " << step.name << std::endl;
    for (auto &step : _steps)
        step->accept(*this);
    std::cerr << "Finished executing grouped step " << step.name << std::endl;
    return nullptr;
}
void *executor::visit(step::confirm &step) {
    std::cerr << step.prompt << std::endl;
    std::cin.get();
    return nullptr;
}
void *executor::visit(step::user_input &step) {
    std::cerr << step.prompt << std::endl;
    std::cin >> step.value;
    while (std::cin.get() != '\n');
    return nullptr;
}
void *executor::visit(step::delay &step) {
    sleep(step.seconds);
    return nullptr;
}
void *executor::visit(step::send &step) {
    _chnls->at(step.channel)->send(step.cmd);
    return nullptr;
}
void *executor::visit(step::recv &step) {
    step.value = _chnls->at(step.channel)->recv_number();
    return nullptr;
}
void *executor::visit(step::math &step) {
    auto value = std::numeric_limits<double>::quiet_NaN();
    switch (step.op) {
        case step::math::ADD:
            value = 0;
            for (auto &o : step.operands)
                value += o(_steps);
            break;
        case step::math::SUB:
            value = 0;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(_steps), flag = false;
                else
                    value -= o(_steps);
            break;
        case step::math::MUL:
            value = 1;
            for (auto &o : step.operands)
                value *= o(_steps);
            break;
        case step::math::DIV:
            value = 1;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(_steps), flag = false;
                else
                    value /= o(_steps);
            break;
    }
    step.value = value;
    return nullptr;
}
