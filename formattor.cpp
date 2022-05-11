#include "formattor.hpp"

#include <iostream>
#include <iomanip>
#include <cmath>

static void engineer_fmt(double v0) {
    std::cout << std::setprecision(5);
    auto v = v0 < 0 ? -v0 : v0;
    if (std::isnan(v0) || v >= 1e27 || v < 1e-24) {
        std::cout << std::scientific << v0;
    } else {
        auto cnt = 0;
        while (v >= 1000)
            v /= 1000, cnt++;
        while (v < 1)
            v *= 1000, cnt--;
        const char *tbl = "YZEPTGMk mμnpfazy";
        std::cout << (v0 < 0 ? '-' : ' ');
        std::cout << std::setw(6) << std::fixed << v;
        std::cout << tbl[8 - cnt];
    }
    std::cout << std::setprecision(6) << std::defaultfloat;
}

void formattor::show() {
    std::cout << "\e[1,1H\e[2J\e[1m[[[ " << profile->name << " ]]]\e[0m\n";
    depth = 0;
    for (auto i = 0zu; auto &st : profile->steps) {
        std::cout << get_prefix(*st) << "[" << i++ << "] ";
        try {
            st->accept(*this);
        } catch (const std::exception &e) {
            std::cout << e.what();
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

const char *formattor::get_prefix(step::step &step) {
    switch (step.status) {
        case step::step::QUEUED:
            return "\e[2m-";
        case step::step::CURRENT:
            return "\e[1m\e[35m*";
        case step::step::FINISHED:
            return "\e[32m+";
        case step::step::REVERTED:
            return "\e[33m-";
    }
    return "?";
}

void *formattor::visit(step::step_group &step) {
    std::cout << step.name;
    switch (step.status) {
        case step::step::CURRENT:
            break;
        case step::step::REVERTED:
        case step::step::FINISHED:
            if (auto st = dynamic_cast<step::valued_step *>(step::get(step.steps, step.digest)); st) {
                switch (st->status) {
                    case step::step::QUEUED:
                    case step::step::CURRENT:
                        goto bad;
                    case step::step::FINISHED:
                        std::cout << "\e[1m";
                    case step::step::REVERTED:
                        std::cout << " => ";
                        engineer_fmt(st->value);
                        break;
                }
                std::cout << st->unit << "\e[0m";
            }
        case step::step::QUEUED:
bad:
            std::cout << "\e[0m";
            return nullptr;
    }
    std::cout << "\e[0\nm";
    depth++;
    for (auto i = 0zu; auto &st : step.steps) {
        if (i)
            std::cout << (step.vertical ? '\n' : '\t');
        if (step.vertical)
            for (auto j = 0zu; j < depth; j++)
                std::cout << "    ";
        std::cout << get_prefix(*st) << "[" << i++ << "] ";
        try {
            st->accept(*this);
        } catch (const std::exception &e) {
            std::cout << e.what();
        }
    }
    depth--;
    return nullptr;
}

void *formattor::visit(step::confirm &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "<y/n>";
            break;
        case step::step::CURRENT:
            std::cout << "<Y/N?>";
            break;
        case step::step::FINISHED:
        case step::step::REVERTED:
            std::cout << "yes";
            break;
    }
    std::cout << "\e[0m";
    return nullptr;
}

void *formattor::visit(step::user_input &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "<______> ";
            break;
        case step::step::CURRENT:
            std::cout << "<??????> ";
            break;
        case step::step::FINISHED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            engineer_fmt(step.value);
            break;
    }
    std::cout << step.unit << "\e[0m";
    return nullptr;
}

void *formattor::visit(step::delay &step) {
    std::cout << step.name << " (" << step.seconds << "s delay)\e[0m";
    return nullptr;
}

void *formattor::visit(step::send &step) {
    std::cout << step.name << " ->" << step.channel;
    if (step.status == step::step::CURRENT)
        std::cout << " " << step.cmd;
    std::cout << "\e[0m";
    return nullptr;
}

void *formattor::visit(step::recv &step) {
    std::cout << step.name << " " << step.channel << "->";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "________ ";
            break;
        case step::step::CURRENT:
            std::cout << "........ ";
            break;
        case step::step::FINISHED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            engineer_fmt(step.value);
            break;
    }
    std::cout << step.unit << "\e[0m";
    return nullptr;
}

void *formattor::visit(step::recv_str &step) {
    std::cout << step.name << " " << step.channel << "->";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "________ ";
            break;
        case step::step::CURRENT:
            std::cout << "........ ";
            break;
        case step::step::FINISHED:
            std::cout << step.value << "\e[1m";
        case step::step::REVERTED:
            step.value;
            break;
    }
    std::cout << "\e[0m";
    return nullptr;
}

void *formattor::visit(step::math &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << " (expr)  ";
            break;
        case step::step::CURRENT:
            std::cout << " (....)  ";
        case step::step::FINISHED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            engineer_fmt(step.value);
            break;
    }
    std::cout << step.unit << "\e[0m";
    return nullptr;
}
