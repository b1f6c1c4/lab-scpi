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
    if (!n.has_val_tag())
        std::cerr << n;
    auto &&o = n.val_tag();
#define T(ty) \
    do { \
        if (o == "!" #ty) { \
            auto step = new step::ty{}; \
            if (n.has_child("name")) \
                n["name"] >> step->name; \
            else \
                step->name = "\b"; \
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
    T(recv_str);
    T(math);
    throw std::runtime_error{ "Unknown type " + o };
}

bool c4::yml::read(const ryml::NodeRef &n, step::step_group *obj) {
    if (n.has_child("digest"))
        parse_path(n["digest"], obj->digest);
    if (n.has_child("vertical"))
        n["vertical"] >> obj->vertical;
    else
        obj->vertical = true;
    n["steps"] >> obj->steps;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::confirm *obj) {
    if (n.has_child("prompt"))
        n["prompt"] >> obj->prompt;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::user_input *obj) {
    if (n.has_child("prompt"))
        n["prompt"] >> obj->prompt;
    if (n.has_child("unit"))
        n["unit"] >> obj->unit;
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
    if (n.has_child("unit"))
        n["unit"] >> obj->unit;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::recv_str *obj) {
    n["channel"] >> obj->channel;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::math::operand *obj) {
    if (!n.is_quoted()) {
        obj->kind = step::math::operand::DOUBLE;
        n >> obj->value;
    } else {
        obj->kind = step::math::operand::REFERENCE;
        parse_path(n, obj->index);
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
    if (n.has_child("unit"))
        n["unit"] >> obj->unit;
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
