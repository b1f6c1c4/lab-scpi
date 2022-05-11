#include "executor.hpp"

#include <iostream>
#include <limits>
#include <cassert>

// return false: is finished
// return true: more next() required
bool executor::next() {
    depth = 0;
    return exec(profile->steps);
}

// return false: shall increment counter
// return true: shall not increment counter
bool executor::exec(step::steps_t &steps) {
    auto is_new = depth == profile->current.size();
    if (is_new)
        profile->current.push_back(0);
    assert(depth < profile->current.size());
    auto &pcd = profile->current[depth];
    if (pcd == steps.size()) {
        profile->current.pop_back();
        return false;
    }
    steps[pcd]->status = step::step::CURRENT;
    if (is_new && dynamic_cast<step::step_group *>(steps[pcd].get()))
        return true;
    ns = &steps;
    if (!steps[pcd]->accept(*this)) {
        steps[pcd]->status = step::step::FINISHED;
        pcd++;
    }
    if (pcd < steps.size()) {
        steps[pcd]->status = step::step::CURRENT;
        return true;
    }
    assert(depth == profile->current.size() - 1);
    profile->current.pop_back();
    return false;
}

void *executor::visit(step::step_group &step) {
    depth++;
    auto ans = exec(step.steps);
    depth--;
    return ans ? this : nullptr;
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
    chnls->at(step.channel)->send(step.cmd);
    return nullptr;
}

void *executor::visit(step::recv &step) {
    step.value = chnls->at(step.channel)->recv_number();
    return nullptr;
}

void *executor::visit(step::recv_str &step) {
    step.value = chnls->at(step.channel)->recv();
    return nullptr;
}

void *executor::visit(step::math &step) {
    auto value = std::numeric_limits<double>::quiet_NaN();
    switch (step.op) {
        case step::math::ADD:
            value = 0;
            for (auto &o : step.operands)
                value += o(*ns);
            break;
        case step::math::SUB:
            value = 0;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(*ns), flag = false;
                else
                    value -= o(*ns);
            break;
        case step::math::MUL:
            value = 1;
            for (auto &o : step.operands)
                value *= o(*ns);
            break;
        case step::math::DIV:
            value = 1;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(*ns), flag = false;
                else
                    value /= o(*ns);
            break;
    }
    step.value = value;
    return nullptr;
}
