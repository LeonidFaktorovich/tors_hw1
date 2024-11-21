#include <common.hpp>

#include <async_connection.hpp>
#include <stdexcept>

CalculateId CalculateId::ParseFromConnection(TcpClient& conn) {
    std::uint32_t size;
    if (!conn.Read(size)) {
        throw std::runtime_error("Can not parse id size");
    }
    if (size != 8) {
        throw std::runtime_error("Can not parse calculate id");
    }
    
    std::uint32_t major;
    if (!conn.Read(major)) {
        throw std::runtime_error("Can not parse major id");
    }
    std::uint32_t minor;
    if (!conn.Read(minor)) {
        throw std::runtime_error("Can not parse minor id");
    }
    return CalculateId(major, minor);
}

CalculateId::CalculateId(std::uint32_t major, std::uint32_t minor) : major_(major), minor_(minor) {
}

std::uint32_t CalculateId::GetMajor() const {
    return major_;
}

std::uint32_t CalculateId::GetMinor() const {
    return minor_;
}
