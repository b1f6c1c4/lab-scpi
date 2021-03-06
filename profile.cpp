#include "profile.hpp"
#include "yaml.hpp"

#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <json.hpp>

static void parse_path(const ryml::NodeRef &n, step::ref_t &path) {
    path.clear();
    auto cs = n.val();
    auto v = 0z;
    auto prev = 0zu;
    auto save = [&](size_t i) {
        if (prev == i) {
            path.emplace_back();
        } else if (v >= 0z) {
            path.emplace_back(static_cast<size_t>(v));
        } else {
            std::string str;
            str.resize(i - prev);
            std::memcpy(str.data(), &cs.str[prev], i - prev);
            path.emplace_back(std::move(str));
        }
    };
    for (auto i = 0zu; i < cs.len; i++) {
        auto ch = cs.str[i];
        if (ch >= '0' && ch <= '9') {
            v *= 10, v += ch - '0';
        } else if (ch == '.') {
            save(i);
            v = 0z, prev = i + 1;
        } else {
            v = -1z;
        }
    }
    save(cs.len);
}

bool c4::yml::read(const ryml::NodeRef &n0, std::unique_ptr<step::step> *obj) {
    auto &n = n0.has_val_tag() ? n0 : n0[0];
    auto &&o = n.val_tag();
#define T(ty) \
    do { \
        if (o == "!" #ty) { \
            auto step = new step::ty{}; \
            if (!n0.has_val_tag()) \
                step->id = n0[0].key(); \
            else \
                step->id = ""; \
            if (n.is_map() && n.has_child("name")) \
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
#undef T
    throw std::runtime_error{ "Unknown type " + o };
}

bool c4::yml::read(const NodeRef &n, step::steps_t *obj) {
    return c4::yml::read(n, static_cast<std::vector<std::unique_ptr<step::step>> *>(obj));
}

bool c4::yml::read(const ryml::NodeRef &n, step::step_group *obj) {
    if (n.has_child("digest"))
        parse_path(n["digest"], obj->digest);
    else
        obj->digest.clear();
    if (n.has_child("vertical"))
        n["vertical"] >> obj->vertical;
    else
        obj->vertical = true;
    n["steps"] >> obj->steps;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::confirm *obj) {
    if (!n.is_map()) {
        obj->prompt = n.val();
        return true;
    }
    if (n.has_child("prompt"))
        n["prompt"] >> obj->prompt;
    else
        obj->prompt = "<Y/N?>";
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::user_input *obj) {
    if (n.has_child("prompt"))
        n["prompt"] >> obj->prompt;
    else
        obj->prompt = "<Y/N?>";
    if (n.has_child("unit"))
        n["unit"] >> obj->unit;
    else
        obj->unit = "";
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::delay *obj) {
    if (n.has_child("seconds"))
        n["seconds"] >> obj->seconds;
    else
        obj->seconds = 0;
    if (n.has_child("nanoseconds"))
        n["nanoseconds"] >> obj->nanoseconds;
    else
        obj->nanoseconds = 0;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::send *obj) {
    n["ch"] >> obj->channel;
    n["cmd"] >> obj->cmd;
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::recv *obj) {
    n["ch"] >> obj->channel;
    if (n.has_child("unit"))
        n["unit"] >> obj->unit;
    else
        obj->unit = "";
    return true;
}

bool c4::yml::read(const ryml::NodeRef &n, step::recv_str *obj) {
    n["ch"] >> obj->channel;
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
            auto ptr = get(steps, index);
again:
            if (auto grp = dynamic_cast<step_group *>(ptr); grp && !grp->digest.empty()) {
                ptr = get(grp->steps, grp->digest);
                goto again;
            }
            return dynamic_cast<valued_step *>(ptr)->value;
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

step::step *step::get(steps_t &steps, const ref_t &cont) {
    step *last{};
    for (auto ptr = &steps; auto &&ref : cont) {
        if (ref.index() == 0) {
            ptr = ptr->parent;
            continue;
        }
        if (ref.index() == 1) {
            last = ptr->at(std::get<size_t>(ref)).get();
        } else {
            auto &s = std::get<std::string>(ref);
            for (auto &st : *ptr)
                if (st->id == s) {
                    last = st.get();
                    goto found;
                }
            throw std::runtime_error{ "Cannot find ref " + s };
        }
found:
        if (auto grp = dynamic_cast<step_group *>(last))
            ptr = &grp->steps;
    }
    return last;
}

void step::resolve_parent(steps_t &steps) {
    for (auto &st : steps)
        if (auto grp = dynamic_cast<step_group *>(st.get())) {
            grp->steps.parent = &steps;
            resolve_parent(grp->steps);
        }
}

step::step *profile_t::operator()() {
    step::step *last{};
    for (auto ptr = &steps; auto id : current) {
        last = ptr->at(id).get();
        if (auto grp = dynamic_cast<step::step_group *>(last))
            ptr = &grp->steps;
    }
    return last;
}

namespace nlohmann {

    template <>
    struct adl_serializer<step::steps_t> {
        static void from_json(const json &j, step::steps_t &vec);
        static void to_json(json &j, const step::steps_t &vec);
    };

    template <>
    struct adl_serializer<std::unique_ptr<step::step>> {
        static void from_json(const json &j, std::unique_ptr<step::step> &step) {
            step->status = j["status"];
            if (auto st = dynamic_cast<step::valued_step *>(step.get()); st)
                j["value"].get_to(st->value);
            else if (auto st = dynamic_cast<step::recv_str *>(step.get()); st)
                j["string"].get_to(st->value);
            else if (auto st = dynamic_cast<step::step_group *>(step.get()); st)
                j["steps"].get_to(st->steps);
        }

        static void to_json(json &j, const std::unique_ptr<step::step> &step) {
            j["status"] = step->status;
            if (auto st = dynamic_cast<step::valued_step *>(step.get()); st)
                j["value"] = st->value;
            else if (auto st = dynamic_cast<step::recv_str *>(step.get()); st)
                j["string"] = st->value;
            else if (auto st = dynamic_cast<step::step_group *>(step.get()); st)
                j["steps"] = st->steps;
        }
    };

    void adl_serializer<step::steps_t>::from_json(const json &j, step::steps_t &vec) {
        for (auto &[k, v] : j.items()) {
            for (auto &st : vec)
                if (st->id == k) {
                    v.get_to(st);
                    break;
                }
        }
    }
    void adl_serializer<step::steps_t>::to_json(json &j, const step::steps_t &vec) {
        j = {};
        for (auto &st : vec)
            if (!st->id.empty())
                j[st->id] = st;
    }

}

std::istream &operator>>(std::istream &is, profile_t &profile) {
    nlohmann::json j{};
    is >> j;
    j["name"].get_to(profile.name);
    j["steps"].get_to(profile.steps);
    j["current"].get_to(profile.current);
    return is;
}

std::ostream &operator<<(std::ostream &os, const profile_t &profile) {
    nlohmann::json j{};
    j["name"] = profile.name;
    j["steps"] = profile.steps;
    j["current"] = profile.current;
    return os << j.dump(2) << std::endl;
}
