#include "executor.hpp"

#include <cassert>
#include <ctime>
#include <iostream>
#include <limits>

#include "fancy_cin.hpp"

#define ALL std::numeric_limits<size_t>::max()

executor::executor(chnl_map *chnls, profile_t *profile)
    : _chnls{ chnls }, _profile{ profile } { }

void executor::start() {
    _limit = 0;
}

void executor::step_in() {
    _limit = ALL;
}

void executor::step_over() {
    _limit = _profile->current.size() + 1;
}

void executor::step_out() {
    _limit = _profile->current.size();
}

void executor::reset() {
    _profile->current.clear();
    _depth = 0;
    revt(_profile->steps);
}

void executor::reverse_step_in() {
    if (_profile->current.empty())
        return;
    if (_profile->current.back()-- == 0)
        _profile->current.pop_back();
    else if (auto grp = dynamic_cast<step::step_group *>(_profile->operator()()))
        _profile->current.push_back(grp->steps.size());
    _depth = 0;
    revt(_profile->steps);
}

void executor::reverse_step_over() {
    if (_profile->current.empty())
        return;
    if (_profile->current.back()-- == 0)
        _profile->current.pop_back();
    _depth = 0;
    revt(_profile->steps);
}

void executor::reverse_step_out() {
    if (_profile->current.empty())
        return;
    _profile->current.pop_back();
    _depth = 0;
    revt(_profile->steps);
}

// return false: has concrete step reverted
// return true: has no concrete step reverted
bool executor::revt(step::steps_t &steps) {
    auto has_concrete = false;
    auto first = 0zu;
    auto has_special = _depth < _profile->current.size();
    if (has_special) {
        first = _profile->current[_depth];
        if (first < steps.size())
            steps[first]->status = step::step::CURRENT;
    }
    for (auto i = first; i < steps.size(); i++) {
        switch (steps[i]->status) {
            case step::step::FINISHED:
                steps[i]->status = step::step::REVERTED;
            case step::step::REVERTED:
                has_concrete = true;
                break;
            case step::step::CURRENT:
                auto child = false;
                if (auto grp = dynamic_cast<step::step_group *>(steps[i].get())) {
                    _depth++;
                    child = revt(grp->steps);
                    _depth--;
                }
                steps[i]->status = child ? step::step::REVERTED : step::step::QUEUED;
                break;
        }
        if (has_special && i == first)
            steps[i]->status = step::step::CURRENT;
    }
    return has_concrete;
}

// return false: no more run needed
// return true: more run needed
bool executor::run() {
    if (!_profile->current.empty() && _profile->current.front() == _profile->steps.size())
        return false;
    _depth = 0;
    if (exec(_profile->steps) == -1)
        return false;
    return _profile->current.size() >= _limit;
}

// return -1: interrupted, shall not incr counter
// return 0: shall increment counter
// return 1: shall not increment counter
int executor::exec(step::steps_t &steps) {
    auto is_new = _depth == _profile->current.size();
    if (is_new)
        _profile->current.push_back(0);
    assert(_depth < _profile->current.size());
    auto &pcd = _profile->current[_depth];
    if (pcd >= steps.size()) {
        if (!_depth)
            return -1;
        _profile->current.pop_back();
        return 0;
    }
    steps[pcd]->status = step::step::CURRENT;
    if (is_new && !dynamic_cast<step::step_group *>(steps[pcd].get()))
        return 1;
    _ns = &steps;
    if (auto ans = steps[pcd]->accept(*this))
        return ans;
    steps[pcd]->status = step::step::FINISHED;
    pcd++;
    if (pcd < steps.size()) {
        steps[pcd]->status = step::step::CURRENT;
        return 1;
    }
    assert(_depth == _profile->current.size() - 1);
    return 0;
}

int executor::visit(step::step_group &step) {
    _depth++;
    auto ans = exec(step.steps);
    _depth--;
    return ans;
}

int executor::visit(step::confirm &step) {
    std::cout << step.prompt << std::endl;
    auto ui = fancy::request_string();
    return ui.kind == fancy::STRING ? 0 : -1;
}

int executor::visit(step::user_input &step) {
    std::cout << step.prompt << std::endl;
    auto ui = fancy::request_double();
    switch (ui.kind) {
        case fancy::DOUBLE:
            step.value = ui.value;
            return 0;
        case fancy::INVALID:
            return 1;
        default:
            return -1;
    }
}

int executor::visit(step::delay &step) {
    timespec rq{
        .tv_sec = step.seconds,
        .tv_nsec = step.nanoseconds,
    };
    return nanosleep(&rq, nullptr) ? -1 : 0;
}

int executor::visit(step::send &step) {
    _chnls->at(step.channel)->send(step.cmd);
    return 0;
}

int executor::visit(step::recv &step) {
    step.value = _chnls->at(step.channel)->recv_number();
    return 0;
}

int executor::visit(step::recv_str &step) {
    step.value = _chnls->at(step.channel)->recv();
    return 0;
}

int executor::visit(step::math &step) {
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
    return 0;
}
