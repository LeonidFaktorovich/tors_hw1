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
#include <sstream>

#include <serialize.hpp>
#include <common.hpp>
#include <tcp_server.hpp>
#include <udp_server.hpp>

std::vector<std::byte> Serialize(int val, formats::To<int>) {
    std::vector<std::byte> str;
    str.resize(sizeof(val));
    std::memcpy(str.data(), &val, sizeof(val));
    return str;
}

std::vector<std::byte> Serialize(double val, formats::To<double>) {
    auto str = std::to_string(val);
    std::vector<std::byte> result;
    for (char c : str) {
        result.push_back(std::byte(c));
    }
    return result;
}

struct Point {
    double x;
    double y;
};

std::vector<Point> ParsePoints(Connection& conn) {
    std::vector<Point> result;

    std::uint32_t n;
    if (!conn.Read(n)) {
        throw std::runtime_error("Can not parse rects number");
    }

    result.resize(n);
    if (!conn.ReadN(result.data(), n)) {
        throw std::runtime_error("Can not parse rects");
    }
    return result;
}

class CalculateId {
public:
    static std::optional<CalculateId> ParseFromConnection(Connection& conn) {
        std::uint32_t size;
        if (!conn.Read(size)) {
            return std::nullopt;
        }
        if (size != 8) {
            return std::nullopt;
        }
        
        std::uint32_t major;
        if (!conn.Read(major)) {
            return std::nullopt;
        }
        std::uint32_t minor;
        if (!conn.Read(minor)) {
            return std::nullopt;
        }
        std::cout << "Parse calculate id: " << major << ", " << minor << '\n';
        return CalculateId(major, minor);
    }

    explicit CalculateId(std::uint32_t major, std::uint32_t minor) : major_(major), minor_(minor){}

    std::uint32_t GetMajor() const {
        return major_;
    }
    std::uint32_t GetMinor() const {
        return minor_;
    }

private:
    std::uint32_t major_;
    std::uint32_t minor_;
};

std::vector<std::byte> Serialize(const CalculateId& id, formats::To<CalculateId>) {
    std::vector<std::byte>result;
    result.resize(12);
    std::uint32_t size = 8;
    std::memcpy(result.data(), &size, 4);

    std::uint32_t major = id.GetMajor();
    std::memcpy(result.data() + 4, &major, 4);
    std::uint32_t minor = id.GetMinor();
    std::memcpy(result.data() + 8, &minor, 4);
    return result;
}

double CalculateIntegral(const std::vector<Point>& points) {
    double result = 0;
    if (points.empty()) {
        return result;
    }
    for (std::size_t i = 0; i < points.size() - 1; ++i) {
        result += (points[i + 1].x - points[i].x) * std::max(points[i].y, points[i + 1].y);
    }
    return result;
}

std::optional<sockaddr_in> ParseAddress(const std::string& data) {
    sockaddr_in addr;
    std::istringstream iss(data);
    std::string ip;
    int port;

    if (!(iss >> ip >> port)) {
        return std::nullopt;
    }
    std::cout << "Parsed adress: {ip: " << ip << ", port: " << port << "}\n";

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        return std::nullopt;
    }

    return addr;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        throw std::runtime_error("No ports");
    }

    std::uint16_t udp_port = std::stoi(std::string{argv[1]});
    std::uint16_t tcp_port = std::stoi(std::string{argv[2]});

    std::thread udp_server([udp_port, tcp_port] {
        try {
            UdpServer server(udp_port);
            char buffer[1024] = {};
            for (;;) {
                auto [addr, n] = server.RecvFrom(buffer, 1024);
                std::cout << "Receive " << n << " bytes. Ip: " << ParseClientAddr(addr).ip << '\n';
                auto client_addr = ParseAddress(std::string(buffer, n));
                if (!client_addr.has_value()) {
                    std::cout << "Failed to parse client addr\n";
                }
                server.SendTo(&tcp_port, 2, *client_addr);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }

    });
    TcpServer tcp_server(tcp_port, 50);
    for (;;) {
        auto conn = tcp_server.AcceptConnection();
        std::cout << "ip: " << conn.GetClientAddr().ip << ", port: " << conn.GetClientAddr().port << '\n';
        for (;;) {
            auto id = CalculateId::ParseFromConnection(conn);
            if (!id.has_value()) {
                conn.Close();
                std::cout << "Close connection to " << "ip: " << conn.GetClientAddr().ip << ", port: " << conn.GetClientAddr().port << '\n';
                break;
            }
            std::cout << "calculate id: " << id->GetMinor() << '\n';
            auto points = ParsePoints(conn);
            // for (const auto& p : points) {
            //     std::cout << "x: " << p.x << ", y: "  << p.y << '\n';
            // }
            std::cout << "read: " << points.size() << " points" << '\n';
            if (!conn.Write(*id)) {
                throw std::runtime_error("Error with write id");
            }
            auto result = CalculateIntegral(points);
            std::cout << "result: " << result << '\n';
            if (!conn.Write(result)) {
                throw std::runtime_error("Error with write");
            }
        }
    }
    udp_server.join();
}