#pragma once

#include <asm-generic/socket.h>
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

#include <common.hpp>

class UdpServer {
public:
    explicit UdpServer(std::uint16_t port);

    struct RecvResult {
        sockaddr_in client_addr;
        ssize_t receive_bytes;
    };

    template <typename T>
    RecvResult RecvFrom(T* value, std::size_t size) {
        sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        
        int n = recvfrom(socket_fd_, value, size, 0, reinterpret_cast<sockaddr*>(&client_addr), &addr_size);
        return RecvResult{client_addr, n};
    }

    template <typename T>
    ssize_t SendTo(const T* value, std::size_t size, const sockaddr_in& client_addr) {
        return sendto(socket_fd_, value, size, 0, reinterpret_cast<const sockaddr*>(&client_addr), sizeof(client_addr));
    }

private:
    int socket_fd_;
};