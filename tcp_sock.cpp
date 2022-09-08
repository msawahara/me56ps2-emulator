#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread>

#include "tcp_sock.h"

void tcp_sock::recv_thread(void)
{
    char buf[64];
    memset(buf, 0, sizeof(buf));

    if (debug_level >= 1) {printf("tcp_sock: start recv_thread.\n");}
    while (true) {
        auto len = ::recv(comm_fd, buf, sizeof(buf), 0);
        if (len > 0) {
            if (debug_level >= 2) {printf("tcp_sock: received %ld bytes.\n", len);}
            (*recv_callback)(buf, len);
        }
    }
}

void tcp_sock::listen_thread(void)
{
    if (debug_level >= 1) {printf("tcp_sock: start listen_thread.\n");}
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        auto client_fd = accept(fd, reinterpret_cast<struct sockaddr *>(&client_addr), &len);

        if (debug_level >= 1) {printf("tcp_sock: client connected.\n");}

        if (client_fd < 0) {
            throw std::runtime_error((std::string) "accept(): " + std::strerror(errno));
        }

        if (comm_fd == 0) {
            comm_fd = client_fd;
            (*ring_callback)();
            new std::thread([&]{tcp_sock::recv_thread();});
        } else {
            ::close(client_fd);
        }
    }
}

tcp_sock::tcp_sock(bool is_server,  const char *ip_addr, uint16_t port)
{
    int ret;
    comm_fd = 0;
    tcp_sock::is_server = is_server;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_addr);

    if (is_server) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw std::runtime_error((std::string) "socket(): " + std::strerror(errno));
        }

        ret = bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        if (ret < 0) {
            throw std::runtime_error((std::string) "bind(): " + std::strerror(errno));
        }

        ret = listen(fd, SOMAXCONN); 
        if (ret < 0) {
            close(fd);
            throw std::runtime_error((std::string) "listen(): " + std::strerror(errno));
        }

        new std::thread([&]{listen_thread();});
    }
}

tcp_sock::~tcp_sock()
{
    if (comm_fd != 0) {
        close(comm_fd);
    }
    if (fd != 0) {
        close(fd);
    }
}

void tcp_sock::set_debug_level(const int level)
{
    debug_level = level;
}

void tcp_sock::set_ring_callback(void (*func)(void))
{
    ring_callback = func;
}

void tcp_sock::set_recv_callback(void (*func)(const char *, size_t))
{
    recv_callback = func;
}

bool tcp_sock::connect()
{
    int ret;
    comm_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (comm_fd < 0) {
        throw std::runtime_error((std::string) "socket(): " + std::strerror(errno));
    }

    ret = ::connect(comm_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (ret < 0) {
        printf("connect(): %s\n", std::strerror(errno));
        return false;
    }
    
    new std::thread([&]{tcp_sock::recv_thread();});
    return true;
}

void tcp_sock::send(const char *buffer, size_t length)
{
    size_t ptr = 0;
    while (ptr < length) {
        ptr += ::send(comm_fd, buffer + ptr, length - ptr, 0);
    }
}

int tcp_sock::recv(char *buffer, size_t max_length)
{
    int ret = ::recv(comm_fd, buffer, max_length, 0);
    return ret;
}
