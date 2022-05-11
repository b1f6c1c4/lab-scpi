#include "profile.hpp"
#include "yaml.hpp"

#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>

static void parse_path(const ryml::NodeRef &n, std::vector<size_t> &path) {
    std::string str;
    n >> str;
    std::stringstream ss{ str };
    while (!ss.eof()) {
        std::string line;
        std::getline(ss, line, '.');
        if (line.size())
            path.push_back(std::stoul(line));
    }
}

bool c4::yml::read(const ryml::NodeRef &n, std::unique_ptr<step::step> *obj) {
    auto &&o = n["type"].val();
#define T(ty) \
    do { \
        if (o == #ty) { \
            auto step = new step::ty{}; \
            n["name"] >> step->name; \
            step->status = step::step::QUEUED; \
            c4::yml::read(n, step); \
            *obj = std::unique_ptr<step::step>{ step }; \
            return true; \
        } \
    } while (false)
    T(step_group);
    T(confirm);
    T(user_input);
    T(delay);
    T(send);
    T(recv);
    T(math);
    throw std::runtime_error{ "Unknown type " + o };
}

bool c4::yml::read(const ryml::NodeRef &n, step::step_group *obj) {
    if (auto &&d = n["digest"]; d.has_val())
        parse_path(d, obj->digest);
    n["vertical"] >> obj->vertical;
    n["steps"] >> obj->steps;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::confirm *obj) {
    n["prompt"] >> obj->prompt;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::user_input *obj) {
    n["prompt"] >> obj->prompt;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::delay *obj) {
    n["seconds"] >> obj->seconds;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::send *obj) {
    n["channel"] >> obj->channel;
    n["cmd"] >> obj->cmd;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::recv *obj) {
    n["channel"] >> obj->channel;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::math::operand *obj) {
    if (n.has_val()) {
        obj->kind = step::math::operand::DOUBLE;
        n >> obj->value;
    } else {
        obj->kind = step::math::operand::REFERENCE;
        parse_path(n["ref"], obj->index);
    }
    return true;
}
double step::math::operand::operator()(steps_t &steps) {
    switch (kind) {
        case DOUBLE:
            return value;
        case REFERENCE:
            return dynamic_cast<valued_step *>(get(steps, index))->value;
    }
    return std::numeric_limits<double>::quiet_NaN();
}
bool c4::yml::read(const ryml::NodeRef &n, step::math *obj) {
    n["operands"] >> obj->operands;
    auto &&o = n["op"].val();
    if (o == "+") obj->op = step::math::ADD;
    else if (o == "-") obj->op = step::math::SUB;
    else if (o == "*") obj->op = step::math::MUL;
    else if (o == "/") obj->op = step::math::DIV;
    else throw std::runtime_error{ "Unknown op " + o };
    return true;
}

step::step &profile_t::operator()() {
    return *step::get(steps, current);
}
