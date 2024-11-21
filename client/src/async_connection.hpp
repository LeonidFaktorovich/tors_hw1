#pragma once

#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <serialize.hpp>
#include <udp_server.hpp>
#include <common.hpp>

class TcpClient {
public:
    TcpClient(const std::string& domain, std::uint16_t port);

    void EnableTcpKeepalive();

    template <typename T>
    bool Read(T& val) {
        std::size_t read_bytes = 0;
        while(read_bytes < sizeof(val)) {
            auto curr_read_bytes = recv(socket_fd_, reinterpret_cast<char*>(&val) + read_bytes, sizeof(val) - read_bytes, 0);
            if (curr_read_bytes == -1) {
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
            if (curr_read_bytes == -1) {
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
            auto curr_write_bytes = send(socket_fd_, val_str.data() + write_bytes, val_str.size() - write_bytes, MSG_NOSIGNAL);
            if (curr_write_bytes == -1) {
                return false;
            }
            write_bytes += curr_write_bytes;
        }
        return write_bytes == val_str.size();
    }

    ~TcpClient();

private:
    int socket_fd_;
};


class AsyncConnection {
public:
    AsyncConnection(const std::string& domain, std::uint16_t port);
    ~AsyncConnection();

    void SendRequest(Request r);
    std::vector<Result> GetResults();

    void Close();
    bool IsClosed();

private:
    std::optional<Request> EnqueueRequest();

    std::thread worker_;

    std::vector<Result> results_;
    std::mutex results_mutex_;

    bool is_closed_{false};
    std::vector<Request> requests_;
    std::mutex requests_mutex_;
    std::condition_variable cv_;

};