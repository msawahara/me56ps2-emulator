#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>

class tcp_sock {
    private:
        int server_fd;
        std::atomic<int> comm_fd; // communication socket fd
        bool is_server;
        int debug_level = 0;
        struct sockaddr_in addr;
        std::thread *recv_thread_ptr;
        std::thread *listen_thread_ptr;
        void (*ring_callback)(void);
        void (*recv_callback)(const char *, size_t);
        void* recv_thread(void);
        void* listen_thread(void);
    public:
        tcp_sock(bool is_server, const char *ip_addr, uint16_t port);
        ~tcp_sock();
        void set_debug_level(const int level);
        void set_ring_callback(void (*func)(void));
        void set_recv_callback(void (*func)(const char *, size_t));
        bool is_connected();
        bool connect();
        void disconnect();
        void send(const char *buffer, size_t length);
        int recv(char *buffer, size_t max_length);
};
