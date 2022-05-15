#pragma once

#include <map>
#include <memory>
#include <string>
#include <fstream>
#include <ext/stdio_filebuf.h>

class scpi {
public:
    virtual void send(const std::string &s) = 0;
    virtual std::string recv() = 0;
    virtual double recv_number() = 0;
};

class fake_scpi : public scpi {
    std::string _name;

public:
    explicit fake_scpi(std::string name);

    void send(const std::string &s) override;
    std::string recv() override;
    double recv_number() override;
};

class fd_scpi : public scpi {
protected:
    int _fd;

    fd_scpi(int fd);

public:
    virtual ~fd_scpi();

    void send(const std::string &s) override;
    std::string recv() override;
    double recv_number() override;
};

class scpi_tcp : public fd_scpi {
public:
    scpi_tcp(const std::string &host, const std::string &port);
};

class scpi_tty : public fd_scpi {
public:
    scpi_tty(const std::string &dev, int baud, int stop);
};

using chnl_map = std::map<std::string, std::unique_ptr<scpi>>;

namespace c4::yml {
    class NodeRef;
    bool read(const NodeRef &n, std::unique_ptr<scpi> *obj);
}
