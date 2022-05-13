#include "executor.hpp"

#include <iostream>
#include <limits>
#include <cassert>

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
    revt(_profile->steps, ALL);
    _profile->current.clear();
}

void executor::reverse_step_in() {
    auto &c = _profile->current;
    if (c.empty()) return;
    if (!c.back()) {
        c.pop_back();
        if (c.empty()) return;
        revt_one(step::get(_profile->steps, c));
        return;
    }
    revt_one(step::get(_profile->steps, c));
    c.back()--;
}

void executor::reverse_step_over() {
    auto &c = _profile->current;
    if (c.empty()) return;
    if (c.back()) {
        revt_one(step::get(_profile->steps, c));
        c.back()--;
        step::get(_profile->steps, c)->status = step::step::CURRENT;
        return;
    }
    c.pop_back();
    if (c.empty()) return;
    revt_one(step::get(_profile->steps, c));
}

void executor::reverse_step_out() {
    auto &c = _profile->current;
    if (c.empty()) return;
    auto v = c.back();
    c.pop_back();
    revt(dynamic_cast<step::step_group *>(step::get(_profile->steps, c))->steps, v);
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

bool executor::revt_one(step::step *st) {
    if (auto grp = dynamic_cast<step::step_group *>(st);
            grp && revt(grp->steps, ALL)) {
        st->status = step::step::REVERTED;
        return true;
    } else {
        st->status = step::step::QUEUED;
        return false;
    }
}

// return false: shall mark as queued
// return true: shall mark as reverted
bool executor::revt(step::steps_t &steps, size_t ub) {
    auto has_concrete = false;
    for (auto i = 0zu; auto &st : steps)
        if (i++ == ub) {
            has_concrete |= revt_one(st.get());
            break;
        } else {
            st->status = step::step::REVERTED;
            has_concrete = true;
            if (auto grp = dynamic_cast<step::step_group *>(st.get()))
                revt(grp->steps, ALL);
        }
    return has_concrete;
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
    if (pcd == steps.size()) {
        _profile->current.pop_back();
        return 0;
    }
    steps[pcd]->status = step::step::CURRENT;
    if (is_new && !dynamic_cast<step::step_group *>(steps[pcd].get()))
        return 1;
    _ns = &steps;
    if (auto ans = steps[pcd]->accept(*this)) {
        return ans;
    }
    steps[pcd]->status = step::step::FINISHED;
    pcd++;
    if (pcd < steps.size()) {
        steps[pcd]->status = step::step::CURRENT;
        return 1;
    }
    assert(_depth == _profile->current.size() - 1);
    _profile->current.pop_back();
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
    return sleep(step.seconds) ? -1 : 0;
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
