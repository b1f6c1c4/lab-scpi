#include "profile.hpp"

#include <cstring>
#include <iostream>
#include <limits>

namespace c4::yml {
    class NodeRef;
    bool read(const NodeRef &n, std::unique_ptr<step::step> *obj);
    bool read(const NodeRef &n, step::confirm *obj);
    bool read(const NodeRef &n, step::delay *obj);
    bool read(const NodeRef &n, step::send *obj);
    bool read(const NodeRef &n, step::recv *obj);
    bool read(const NodeRef &n, step::math *obj);
    bool read(const NodeRef &n, step::math::operand *obj);
    bool read(const NodeRef &n, std::unique_ptr<scpi> *obj);
    bool read(const NodeRef &n, profile *obj);
}

#include <rapidyaml.hpp>

using namespace std::string_literals;

std::string operator+(const char *s, c4::csubstr buf) {
    auto len = std::strlen(s);
    std::string ans;
    ans.resize(len + buf.len);
    memcpy(&ans[0], s, len);
    memcpy(&ans[len], buf.str, buf.len);
    return ans;
}

bool c4::yml::read(const ryml::NodeRef &n, std::unique_ptr<step::step> *obj) {
    auto &&o = n["type"].val();
#define T(ty) \
    do { \
        if (o == #ty) { \
            auto step = new step::ty{}; \
            c4::yml::read(n, step); \
            *obj = std::unique_ptr<step::step>{ step }; \
            return true; \
        } \
    } while (false)
    T(confirm);
    T(delay);
    T(send);
    T(recv);
    T(math);
    throw std::runtime_error{ "Unknown type " + o };
}

void step::confirm::execute(profile *pf) {
    std::cout << "Press enter to continue ...\n";
    std::cin.get();
}
bool c4::yml::read(const ryml::NodeRef &n, step::confirm *obj) {
    n["prompt"] >> obj->prompt;
    return true;
}

void step::delay::execute(profile *pf) {
    sleep(seconds);
}
bool c4::yml::read(const ryml::NodeRef &n, step::delay *obj) {
    n["seconds"] >> obj->seconds;
    return true;
}

void step::send::execute(profile *pf) {
    pf->chnls.at(channel)->send(cmd);
}
bool c4::yml::read(const ryml::NodeRef &n, step::send *obj) {
    n["channel"] >> obj->channel;
    n["cmd"] >> obj->cmd;
    return true;
}

void step::recv::execute(profile *pf) {
    value = pf->chnls.at(channel)->recv_number();
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
        n["ref"] >> obj->index;
    }
    return true;
}
double step::math::operand::operator()(profile *pf) const {
    switch (kind) {
        case DOUBLE:
            return value;
        case REFERENCE:
            return dynamic_cast<valued_step *>(pf->steps[index].get())->value;
    }
    return std::numeric_limits<double>::quiet_NaN();
}
void step::math::execute(profile *pf) {
    switch (op) {
        case ADD:
            value = 0;
            for (auto &o : operands)
                value += o(pf);
            break;
        case SUB:
            value = 0;
            for (auto flag = true; auto &o : operands)
                if (flag)
                    value = o(pf), flag = false;
                else
                    value -= o(pf);
            break;
        case MUL:
            value = 1;
            for (auto &o : operands)
                value *= o(pf);
            break;
        case DIV:
            value = 1;
            for (auto flag = true; auto &o : operands)
                if (flag)
                    value = o(pf), flag = false;
                else
                    value /= o(pf);
            break;
    }
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

bool c4::yml::read(const ryml::NodeRef &n, std::unique_ptr<scpi> *obj) {
    if (n.has_val()) {
        auto &&o = n["op"].val();
        if (o == "virtual") obj->reset();
        else throw std::runtime_error{ "Unknown scpi " + o };
    } else {
        auto &&o = n[0].key();
        if (o == "tcp") {
            std::string host;
            int port;
            n[0] >> host >> port;
            *obj = std::make_unique<scpi_tcp>(host, port);
        } else if (o == "tty") {
            std::string dev;
            n[0] >> dev;
            *obj = std::make_unique<scpi_tty>(dev);
        } else throw std::runtime_error{ "Unknown scpi " + o };
    }
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, profile *obj) {
    n["channels"] >> obj->chnls;
    n["steps"] >> obj->steps;
    return true;
}
