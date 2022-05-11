#include "formattor.hpp"

#include <iostream>
#include <iomanip>
#include <cmath>

static void fmt(double v0) {
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
    std::cout << step.name << "\e[0m\n";
    for (auto i = 0zu; i < depth; i++)
        std::cout << "    ";
    depth++;
    auto i = 0zu;
    for (auto &st : step.steps) {
        std::cout << get_prefix(step) << "[" << i++ << "] ";
        st->accept(*this);
    }
    depth--;
    return nullptr;
}

void *formattor::visit(step::confirm &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
        case step::step::REVERTED:
            std::cout << "yes";
            break;
        case step::step::CURRENT:
            std::cout << "<Y/N?>";
            break;
        case step::step::FINISHED:
            std::cout << "<y/n>";
            break;
    }
    std::cout << "\e[0m\n";
    return nullptr;
}

void *formattor::visit(step::user_input &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            fmt(step.value);
            break;
        case step::step::CURRENT:
            std::cout << "<??????> ";
            break;
        case step::step::FINISHED:
            std::cout << "<______> ";
            break;
    }
    std::cout << step.unit << "\e[0m\n";
    return nullptr;
}

void *formattor::visit(step::delay &step) {
    std::cout << step.name << " (" << step.seconds << "s dly)\e[0m\n";
    return nullptr;
}

void *formattor::visit(step::send &step) {
    std::cout << step.name << " -> " << step.channel << " ";
    std::cout << " " << step.cmd << "\e[0m\n";
    return nullptr;
}

void *formattor::visit(step::recv &step) {
    std::cout << step.name << " <- " << step.channel << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            fmt(step.value);
            break;
        case step::step::CURRENT:
            std::cout << "........ ";
            break;
        case step::step::FINISHED:
            std::cout << "________ ";
            break;
    }
    std::cout << step.unit << "\e[0m\n";
    return nullptr;
}

void *formattor::visit(step::math &step) {
    std::cout << step.name << " ";
    switch (step.status) {
        case step::step::QUEUED:
            std::cout << "\e[1m";
        case step::step::REVERTED:
            fmt(step.value);
            break;
        case step::step::CURRENT:
        case step::step::FINISHED:
            std::cout << " (expr)  ";
            break;
    }
    std::cout << step.unit << "\e[0m\n";
    return nullptr;
}
