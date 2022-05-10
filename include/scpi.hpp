#include <string>
#include <fstream>
#include <ext/stdio_filebuf.h>

class scpi {
protected:
    int _fd;
    __gnu_cxx::stdio_filebuf<char> _fb;
    std::iostream _fs;

    scpi(int fd);

public:
    virtual ~scpi();

    void send(std::string s);
    std::string recv();
    double recv_number();
};

class scpi_tcp : public scpi {
public:
    scpi_tcp(const std::string &host, int port);
};

class scpi_tty : public scpi {
public:
    scpi_tty(const std::string &dev);
};
