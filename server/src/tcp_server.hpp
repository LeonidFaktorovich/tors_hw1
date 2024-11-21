#include <asm-generic/socket.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <optional>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <thread>

#include <serialize.hpp>
#include <common.hpp>

class Connection {
public:
    Connection(int socket_fd, sockaddr_storage client_addr) : socket_fd_(socket_fd), client_addr_(ParseClientAddr(client_addr)) {}

    ClientAddr GetClientAddr() {
        return client_addr_;
    }

    template <typename T>
    bool Read(T& val) {
        std::size_t read_bytes = 0;
        while(read_bytes < sizeof(val)) {
            auto curr_read_bytes = recv(socket_fd_, reinterpret_cast<char*>(&val) + read_bytes, sizeof(val) - read_bytes, 0);
            if (curr_read_bytes <= 0) {
                return false;
            }
            read_bytes += curr_read_bytes;
        }
        return true;
    }

    template <typename T>
    bool ReadN(T* array, std::size_t n) {
        std::size_t read_bytes = 0;
        const std::size_t need_read = sizeof(*array) * n;
        while (read_bytes < need_read) {
            auto curr_read_bytes = recv(socket_fd_, reinterpret_cast<char*>(array) + read_bytes, need_read - read_bytes, 0);
            if (curr_read_bytes <= 0) {
                return false;
            }
            read_bytes += curr_read_bytes;
        }
        return true;
    }

    template <typename T>
    bool Write(const T& val) {
        auto val_str = formats::Serialize(val);
        std::size_t write_bytes = 0;
        while (write_bytes < val_str.size()) {
            auto curr_write_bytes = send(socket_fd_, val_str.data() + write_bytes, val_str.size() - write_bytes, 0);
            if (curr_write_bytes == -1) {
                return false;
            }
            write_bytes += curr_write_bytes;
        }
        return write_bytes == val_str.size();
    }

    ~Connection() {
        close(socket_fd_);
    }

    void Close() {
        close(socket_fd_);
    }

private: 
    int socket_fd_;
    ClientAddr client_addr_;
};

class TcpServer {
public:
    TcpServer(uint16_t port, int connections_buffer_size) {
        addrinfo hints;
        addrinfo* result = NULL;
        addrinfo* rp = NULL;
        std::memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string port_str = std::to_string(port);
        int s = getaddrinfo(NULL, port_str.c_str(), &hints, &result);
        if (s != 0) {
            throw std::runtime_error("Error with getaddrinfo: " + std::string(gai_strerror(s)));
        }
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            socket_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (socket_fd_ == -1) {
                continue;
            }
            if (bind(socket_fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
                
                break;
            }
            close(socket_fd_);
        }
        freeaddrinfo(result);
        int enable_reuse = 1;
        setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(enable_reuse));
        if (rp == NULL) {
            throw std::runtime_error("Error with bind");
        }


        if (listen(socket_fd_, connections_buffer_size) == 1) {
            throw std::runtime_error("Error with listen");
        }
    }

    ~TcpServer() {
        close(socket_fd_);
    }

    Connection AcceptConnection() {
        sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);

        int connfd = accept(socket_fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_size);
        if (connfd == -1) {
            throw std::runtime_error("Error with accept");
        }
        return Connection(connfd, client_addr);
    }

private:
    int socket_fd_;
};
