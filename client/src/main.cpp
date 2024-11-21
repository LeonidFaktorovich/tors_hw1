#include <chrono>
#include <numeric>
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
#include <unordered_set>

#include <serialize.hpp>
#include <udp_server.hpp>
#include <async_connection.hpp>
#include <common.hpp>

struct ClientAddr {
    std::string ip;
    std::uint16_t port;
};

ClientAddr ParseClientAddr(const sockaddr_in& addr) {
    ClientAddr client_addr;
    client_addr.port = addr.sin_port;

    client_addr.ip.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr.sin_addr, client_addr.ip.data(), client_addr.ip.size());
    return client_addr;
}

Request GetRequest(const std::vector<Point> points, std::size_t i, std::size_t servers_count, std::uint32_t major_id) {
    const std::uint32_t work_size = points.size() / servers_count;
    std::size_t first_point = work_size * i;
    std::size_t last_point = (i + 1 != servers_count ? work_size * (i + 1) : points.size());
    std::vector<Point> curr_points(points.begin() + first_point, points.begin() + last_point);

    return Request{.points = std::move(curr_points), .id = CalculateId(major_id, i)};
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        throw std::runtime_error("no ports");
    }

    std::uint16_t udp_receiver_port = std::stoul(argv[1]);
    std::uint16_t udp_sender_port = std::stoul(argv[2]);

    UdpServer receiver(udp_receiver_port);
    UdpServer sender(udp_sender_port);
    std::string message = std::string("127.0.0.1") + ' ' + std::to_string(udp_receiver_port);
    sender.EnableBroadcast();

    const std::size_t servers_count = argc - 3;
    for (std::size_t i = 3; i < argc; ++i) {
        sender.SendBroadcast(message.data(), message.size(), std::stoul(argv[i]));
    }

    std::vector<double> servers_results;
    servers_results.resize(servers_count);

    std::vector<std::pair<std::string, std::uint16_t>> servers_addr;
    
    for (std::size_t i = 0; i < servers_count; ++i) {
        std::uint16_t server_port;
        auto [addr, _] = receiver.RecvFrom(&server_port, sizeof(server_port));
        std::string domain = ParseClientAddr(addr).ip;
        std::uint16_t port = server_port;
        std::cout << "Server domain: " << domain << ", port: " << port << "\n";
        servers_addr.push_back(std::make_pair(std::move(domain), port));
    }

    
    std::vector<Point> points;
    for (std::size_t i = 0; i < 100'000; ++i) {
        double step = 1 / double(100'000);
        double x = 1.0 + step * i;
        double y = x * x;
        points.push_back(Point{x, y});
    }

    const std::uint32_t major_id = 42;
    std::vector<std::unique_ptr<AsyncConnection>> connections;
    const std::uint32_t work_size = points.size() / servers_count;
    for (std::size_t i = 0; i < servers_count; ++i) {
        connections.push_back(std::make_unique<AsyncConnection>(servers_addr[i].first, servers_addr[i].second));

        connections.back()->SendRequest(GetRequest(points, i, servers_count, major_id));
    }


    std::vector<std::optional<double>> parts(connections.size(), std::nullopt);
    std::uint32_t calc_parts = 0;
    std::vector<std::uint32_t> waits(servers_count, 1);
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        for (std::size_t i = 0; i < connections.size(); ++i) {
            if (waits[i] == 0) {
                continue;
            }
            auto results = connections[i]->GetResults();
            if (results.empty()) {
                std::cout << "Failed connection: " << i << '\n';
                continue;
            }

            waits[i] -= results.size();
            for (auto& result : results) {
                std::cout << "Get result for " << result.id.GetMinor() << " from " << i << std::endl;
                if (!parts[result.id.GetMinor()].has_value()) {
                    parts[result.id.GetMinor()] = result.result;
                    calc_parts += 1;
                }
            }
        }

        if (calc_parts == servers_count) {
            break;
        }

        std::size_t part_index = 0;
        while (parts[part_index].has_value()) {
            ++part_index;
        }

        for (std::size_t i = 0; i < servers_count; ++i) {
            if (waits[i] > 0) {
                continue;
            }
            connections[i]->SendRequest(GetRequest(points, part_index, servers_count, major_id));
            std::cout << "Send part " << part_index << " to " << i << std::endl;
            waits[i] = 1;

            while (part_index < parts.size() && parts[part_index].has_value()) {
                ++part_index;
            }
            if (part_index == parts.size()) {
                break;
            }
        }
    }

    for (auto& conn : connections) {
        conn->Close();
    }

    double result = 0;
    for (auto part : parts) {
        result += part.value();
    }
    std::cout << "result: " << result << '\n';
}
