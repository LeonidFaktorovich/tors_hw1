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

class UdpServer {
public:
    explicit UdpServer(std::uint16_t port);
    ~UdpServer();

    struct RecvResult {
        sockaddr_in client_addr;
        ssize_t receive_bytes;
    };

    void EnableBroadcast();

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

    template <typename T>
    ssize_t SendBroadcast(const T* value, std::size_t size, std::uint16_t port) {
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("255.255.255.255");
        addr.sin_port = htons(port);
        std::cout << "Send broadcast to " << port << '\n';

        return SendTo(value, size, addr);
    }

private:
    int socket_fd_;
};