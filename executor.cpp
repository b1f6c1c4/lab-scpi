#include "executor.hpp"

#include <iostream>
#include <limits>
#include <cassert>

#include "fancy_cin.hpp"

// return false: is finished
// return true: more next() required
bool executor::next() {
    _depth = 0;
    return exec(_profile->steps);
}

// return false: shall increment counter
// return true: shall not increment counter
bool executor::exec(step::steps_t &steps) {
    auto is_new = _depth == _profile->current.size();
    if (is_new)
        _profile->current.push_back(0);
    assert(_depth < _profile->current.size());
    auto &pcd = _profile->current[_depth];
    if (pcd == steps.size()) {
        _profile->current.pop_back();
        return false;
    }
    steps[pcd]->status = step::step::CURRENT;
    if (is_new && !dynamic_cast<step::step_group *>(steps[pcd].get()))
        return true;
    _ns = &steps;
    if (!steps[pcd]->accept(*this)) {
        steps[pcd]->status = step::step::FINISHED;
        pcd++;
    }
    if (pcd < steps.size()) {
        steps[pcd]->status = step::step::CURRENT;
        return true;
    }
    assert(_depth == _profile->current.size() - 1);
    _profile->current.pop_back();
    return false;
}

void *executor::visit(step::step_group &step) {
    _depth++;
    auto ans = exec(step.steps);
    _depth--;
    return ans ? this : nullptr;
}

void *executor::visit(step::confirm &step) {
    std::cout << step.prompt << "\n> " << std::flush;
    auto ui = fancy::request_string();
    return ui.kind == fancy::STRING ? nullptr : this;
}

void *executor::visit(step::user_input &step) {
    std::cout << step.prompt << "\n> " << std::flush;
    auto ui = fancy::request_double();
    if (ui.kind != fancy::DOUBLE)
        return this;
    step.value = ui.value;
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

void *executor::visit(step::recv_str &step) {
    step.value = _chnls->at(step.channel)->recv();
    return nullptr;
}

void *executor::visit(step::math &step) {
    auto value = std::numeric_limits<double>::quiet_NaN();
    switch (step.op) {
        case step::math::ADD:
            value = 0;
            for (auto &o : step.operands)
                value += o(*_ns);
            break;
        case step::math::SUB:
            value = 0;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(*_ns), flag = false;
                else
                    value -= o(*_ns);
            break;
        case step::math::MUL:
            value = 1;
            for (auto &o : step.operands)
                value *= o(*_ns);
            break;
        case step::math::DIV:
            value = 1;
            for (auto flag = true; auto &o : step.operands)
                if (flag)
                    value = o(*_ns), flag = false;
                else
                    value /= o(*_ns);
            break;
    }
    step.value = value;
    return nullptr;
}
