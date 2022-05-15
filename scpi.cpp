#include "scpi.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

#include "yaml.hpp"

using namespace std::string_literals;

fake_scpi::fake_scpi(std::string name) : _name{ std::move(name) } {
    std::cout << "(fake-scpi " << _name << " connected)\n";
}

void fake_scpi::send(const std::string &s) {
    std::cout << "(fake-scpi " << _name << " sending " << s << ")\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
}

std::string fake_scpi::recv() {
    std::cout << "(fake-scpi " << _name << " receiving)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return _name;
}

double fake_scpi::recv_number() {
    std::cout << "(fake-scpi " << _name << " receiving number)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}

fd_scpi::fd_scpi(int fd) : _fd{ fd } { }

void fd_scpi::send(const std::string &s) {
    iovec iov[] = {
        { (void*)s.data(), s.size() },
        { (void*)"\n", 1 },
    };
    writev(_fd, iov, sizeof(iov) / sizeof(iov[0]));
}

fd_scpi::~fd_scpi() {
    close(_fd);
}

std::string fd_scpi::recv() {
    std::string str;
again:
    switch (char buf[257]; auto len = read(_fd, buf, 256)) {
        case 0:
            return "";
        case -1:
            throw std::runtime_error{ "read: "s + std::strerror(errno) };
        default:
            auto prev = str.size();
            str.resize(len);
            memcpy(str.data() + prev, buf, len);
            if (buf[len - 1] != '\n')
                goto again;
            return str;
    }
}

double fd_scpi::recv_number() {
    auto str = recv();
    char *end;
    if (auto val = std::strtof(str.data(), &end); *end)
        return val;
    throw std::runtime_error{ "recv_number: Got " + str };
}

scpi_tcp::scpi_tcp(const std::string &host, const std::string &port) :
    fd_scpi{ [&]{
        int sfd{ socket(AF_INET, SOCK_STREAM, 0) };
        if (sfd < 0)
            throw std::runtime_error{ "socket: "s + std::strerror(errno) };
        static constexpr const addrinfo hint{
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = 0,
        };
        addrinfo *ai;
        if (auto ret = getaddrinfo(host.data(), port.data(), &hint, &ai); ret)
            throw std::runtime_error{ "getaddrinfo: "s + gai_strerror(ret) };
        for (auto ptr = ai; ptr; ptr = ptr->ai_next)
            if (auto ret = connect(sfd, ptr->ai_addr, ptr->ai_addrlen); !ret) {
                freeaddrinfo(ai);
                return sfd;
            } else {
                std::cout << "When connecting to " << host << ": " << std::strerror(errno) << std::endl;
            }
        freeaddrinfo(ai);
        throw std::runtime_error{ "Cannot connect to " + host };
    }() } { }

scpi_tty::scpi_tty(const std::string &dev, int baud, int stop) :
    fd_scpi{ [&]{
        auto fd = open(dev.data(), O_RDWR | O_NOCTTY);
        if (fd < 0)
            throw std::runtime_error{ "open: "s + std::strerror(errno) };
        int flag = TIOCM_DTR;
        if (ioctl(_fd, TIOCMBIC, &flag) == -1)
            throw std::runtime_error{ "ioctl: "s + std::strerror(errno) };
        close(fd);
        fd = open(dev.data(), O_RDWR | O_NOCTTY);
        if (fd < 0)
            throw std::runtime_error{ "open: "s + std::strerror(errno) };
        return fd;
    }() } {
    termios cfg;
    if (tcgetattr(_fd, &cfg) == -1)
        throw std::runtime_error{ "tcgetattr: "s + std::strerror(errno) };
    int b;
    switch (baud) {
        case 0: b = B0; break;
        case 50: b = B50; break;
        case 75: b = B75; break;
        case 110: b = B110; break;
        case 134: b = B134; break;
        case 150: b = B150; break;
        case 200: b = B200; break;
        case 300: b = B300; break;
        case 600: b = B600; break;
        case 1200: b = B1200; break;
        case 1800: b = B1800; break;
        case 2400: b = B2400; break;
        case 4800: b = B4800; break;
        case 9600: b = B9600; break;
        case 19200: b = B19200; break;
        case 38400: b = B38400; break;
        case 57600: b = B57600; break;
        case 115200: b = B115200; break;
        case 230400: b = B230400; break;
        case 460800: b = B460800; break;
        case 500000: b = B500000; break;
        case 576000: b = B576000; break;
        case 921600: b = B921600; break;
        case 1000000: b = B1000000; break;
        case 1152000: b = B1152000; break;
        case 1500000: b = B1500000; break;
        case 2000000: b = B2000000; break;
        case 2500000: b = B2500000; break;
        case 3000000: b = B3000000; break;
        case 3500000: b = B3500000; break;
        case 4000000: b = B4000000; break;
        default: throw std::runtime_error{ "Invalid baud: " + std::to_string(baud) };
    }
    cfsetspeed(&cfg, b);
    switch (stop) {
        case 2:
            cfg.c_cflag |= (CSTOPB);
            break;
        case 1:
            cfg.c_cflag &= ~(CSTOPB);
            break;
        default:
            throw std::runtime_error{ "Invalid stop: " + std::to_string(stop) };
    }
    cfg.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    cfg.c_oflag &= ~(OPOST | OCRNL);
    cfg.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    cfg.c_cflag &= ~(CSIZE | PARENB);
    cfg.c_cflag |= (CS8);
    cfg.c_cc[VMIN] = 1; cfg.c_cc[VTIME] = 0;
    if (tcsetattr(_fd, TCSAFLUSH, &cfg) == -1)
        throw std::runtime_error{ "tcsetattr: "s + std::strerror(errno) };
}

bool c4::yml::read(const ryml::NodeRef &n, std::unique_ptr<scpi> *obj) {
    auto &&o = n[0].key();
    if (o == "tcp") {
        std::string host;
        std::string port;
        n[0]["host"] >> host;
        n[0]["port"] >> port;
        *obj = std::make_unique<scpi_tcp>(host, port);
    } else if (o == "tty") {
        std::string dev;
        auto baud = 9600;
        auto stop = 1;
        n[0]["dev"] >> dev;
        if (n[0].has_child("baud"))
            n[0]["baud"] >> baud;
        if (n[0].has_child("stop"))
            n[0]["stop"] >> stop;
        *obj = std::make_unique<scpi_tty>(dev, baud, stop);
    } else if (o == "virtual") {
        *obj = std::make_unique<fake_scpi>("f-" + n[0].val());
    } else throw std::runtime_error{ "Unknown scpi " + o };
    return true;
}
