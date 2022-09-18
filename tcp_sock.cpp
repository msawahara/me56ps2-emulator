#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread>

#include "tcp_sock.h"

void* tcp_sock::recv_thread(void)
{
    fd_set readfds;
    timeval recv_timeout = {.tv_sec = 0, .tv_usec = 100 * 1000}; // 100ms
    auto comm_fd = tcp_sock::comm_fd.load();
    char buf[64];

    if (debug_level >= 1) {printf("tcp_sock: start recv_thread.\n");}
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(comm_fd, &readfds);
        auto ret = select(comm_fd + 1, &readfds, nullptr, nullptr, &recv_timeout);
        if (ret < 0) {
            printf("tcp_sock: select(): %s\n", std::strerror(errno));
            break;
        }
        if (ret == 0) {
            if (!is_connected()) {
                // Connection closed
                break;
            }
            // No data
            continue;
        }

        auto len = ::recv(comm_fd, buf, sizeof(buf), 0);
        if (len < 0) {
            printf("tcp_sock: recv(): %s\n", std::strerror(errno));
            break;
        }
        if (len == 0) {
            printf("tcp_sock: connection closed.\n");
            break;
        }
        if (debug_level >= 2) {printf("tcp_sock: received %ld bytes.\n", len);}
        (*recv_callback)(buf, len);
    }

    return nullptr;
}

void* tcp_sock::listen_thread(void)
{
    if (debug_level >= 1) {printf("tcp_sock: start listen_thread.\n");}
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        auto client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &len);

        if (debug_level >= 1) {printf("tcp_sock: client connected.\n");}

        if (client_fd < 0) {
            throw std::runtime_error((std::string) "accept(): " + std::strerror(errno));
        }

        if (comm_fd.load() == 0) {
            comm_fd.store(client_fd);
            (*ring_callback)();
            recv_thread_ptr = new std::thread([&]{tcp_sock::recv_thread();});
        } else {
            ::close(client_fd);
        }
    }

    return nullptr;
}

tcp_sock::tcp_sock(bool is_server,  const char *ip_addr, uint16_t port)
{
    int ret;
    comm_fd.store(0);
    tcp_sock::is_server = is_server;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_addr);

    if (is_server) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::runtime_error((std::string) "tcp_sock: socket(): " + std::strerror(errno));
        }

        ret = bind(server_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
        if (ret < 0) {
            throw std::runtime_error((std::string) "tcp_sock: bind(): " + std::strerror(errno));
        }

        ret = listen(server_fd, SOMAXCONN); 
        if (ret < 0) {
            close(server_fd);
            throw std::runtime_error((std::string) "tcp_sock: listen(): " + std::strerror(errno));
        }

        listen_thread_ptr = new std::thread([&]{listen_thread();});
    }
}

tcp_sock::~tcp_sock()
{
    if (server_fd != 0) {
        close(server_fd);
    }
    if (listen_thread_ptr != nullptr) {
        listen_thread_ptr->join();
    }
    disconnect();
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

bool tcp_sock::is_connected()
{
    return comm_fd.load() != 0;
}

bool tcp_sock::connect()
{
    int ret;
    auto comm_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (comm_fd < 0) {
        throw std::runtime_error((std::string) "tcp_sock: socket(): " + std::strerror(errno));
    }

    ret = ::connect(comm_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (ret < 0) {
        printf("tcp_sock: connect(): %s\n", std::strerror(errno));
        return false;
    }
    
    tcp_sock::comm_fd.store(comm_fd);
    recv_thread_ptr = new std::thread([&]{tcp_sock::recv_thread();});
    return true;
}

void tcp_sock::disconnect()
{
    auto comm_fd = tcp_sock::comm_fd.load();
    if (comm_fd != 0) {
        close(comm_fd);
        tcp_sock::comm_fd.store(0);
    }
    if (recv_thread_ptr != nullptr) {
        if (recv_thread_ptr->joinable()) {
            recv_thread_ptr->join();
        }
        recv_thread_ptr = nullptr;
    }
}

void tcp_sock::send(const char *buffer, size_t length)
{
    size_t ptr = 0;
    auto comm_fd = tcp_sock::comm_fd.load();
    if (comm_fd == 0) {
        printf("tcp_sock: socket closed.\n");
        return;
    }
    while (ptr < length) {
        auto ret = ::send(comm_fd, buffer + ptr, length - ptr, MSG_NOSIGNAL);
        if (ret < 0) {
            printf("tcp_sock: send(): %s\n", std::strerror(errno));
            break;
        }
        ptr += ret;
    }
}

int tcp_sock::recv(char *buffer, size_t max_length)
{
    auto comm_fd = tcp_sock::comm_fd.load();
    int ret = ::recv(comm_fd, buffer, max_length, 0);
    if (ret < 0) {
        printf("tcp_sock: recv(): %s\n", std::strerror(errno));
    }
    return ret;
}
