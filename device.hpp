#include "scpi.hpp"

struct device {
    scpi *backend;

    std::string idn() {
        send("*IDN?");
        return recv();
    }

    void rst() {
        send("*RST");
    }
};

// Siglent SDM: port 5025
struct dmm : device {
    double measure_voltage_dc() {
        send("MEAS:VOLT:DC?");
        return recv_number();
    }
    double measure_current_dc() {
        send("MEAS:CURR:DC?");
        return recv_number();
    }
};

// Siglent SDS: port 5025
struct scope : device { };

// Teledyne T3AFG: port 5024
struct afg : device {
    void output(int channel, bool value) {
        auto cmd = channel : "C1"s : "C2"s;
        cmd += ":OUTP ";
        cmd += value ? "ON" : "OFF";
        send(cmd);
    }
};

// TTi TF960
struct counter : device {
    void set_measure_time(int kind) {
        send("M" + std::to_string(kind));
    }
    double measure_frequency() {
        send("N?");
        return recv_number();
    }
};

// Keysight E3632A
struct power : device {
    void show_message(const std::string &str) {
        send("DISP:TEXT \"" + str + "\"");
    }
    void hide_message() {
        send("DISP:TEXT:CLE");
    }
    void beep() {
        send("SYST:BEEP");
    }
    void output(bool value) {
        send(value ? "OUTP ON" : "OUTP OFF");
    }
    double measure_voltage() {
        send("MEAS:VOLT?");
        return recv_number();
    }
    double measure_current() {
        send("MEAS:CURR?");
        return recv_number();
    }
};
