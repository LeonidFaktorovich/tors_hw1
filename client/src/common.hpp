#pragma once

#include <string>
#include <stdexcept>
#include <vector>

class TcpClient;

struct Point {
    double x;
    double y;
};

class CalculateId {
public:
    static CalculateId ParseFromConnection(TcpClient& conn);

    explicit CalculateId(std::uint32_t major, std::uint32_t minor);

    std::uint32_t GetMajor() const;
    std::uint32_t GetMinor() const;

private:
    std::uint32_t major_;
    std::uint32_t minor_;
};

struct Request {
    std::vector<Point> points;
    CalculateId id;
};

struct Result {
    CalculateId id;
    double result;
};