#include "scpi.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "yaml.hpp"

using namespace std::string_literals;

scpi::scpi(int fd) :
    _fd{ fd },
    _fb{ fd, std::ios::in | std::ios::out },
    _fs{ &_fb } { }

void scpi::send(std::string s) {
    _fs << s << std::endl;
}

scpi::~scpi() {
    _fb.close();
}

std::string scpi::recv() {
    std::string str;
    std::getline(_fs, str);
    return str;
}

double scpi::recv_number() {
    double n;
    _fs >> n;
    while (_fs.get() != '\n');
    return n;
}

scpi_tcp::scpi_tcp(const std::string &host, int port) :
    scpi{ [&]{
        int sfd{ socket(AF_INET, SOCK_STREAM, 0) };
        if (sfd < 0)
            throw std::runtime_error{ "socket:"s + std::strerror(errno) };
        static constexpr const addrinfo hint{
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = 0,
        };
        addrinfo *ai;
        if (auto ret = getaddrinfo(host.data(), nullptr, &hint, &ai); ret)
            throw std::runtime_error{ "getaddrinfo:"s + gai_strerror(ret) };
        for (auto ptr = ai; ptr; ptr = ptr->ai_next)
            if (auto ret = connect(sfd, ptr->ai_addr, ptr->ai_addrlen); !ret) {
                freeaddrinfo(ai);
                return sfd;
            }
        freeaddrinfo(ai);
        throw std::runtime_error{ "Cannot connect to " + host };
    }() } { }

scpi_tty::scpi_tty(const std::string &dev) :
    scpi{ [&]{
        auto fd = open(dev.data(), O_RDWR | O_NOCTTY);
        if (fd < 0)
            throw std::runtime_error{ "open:"s + std::strerror(errno) };
        return fd;
    }() } {
    int flag = TIOCM_DTR;
    if (ioctl(_fd, TIOCMBIC, &flag) == -1)
        throw std::runtime_error{ "ioctl:"s + std::strerror(errno) };
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
