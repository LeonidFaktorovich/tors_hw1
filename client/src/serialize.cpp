#include <serialize.hpp>
#include <common.hpp>

namespace formats {

std::vector<std::byte> Serialize(std::uint32_t val, formats::To<std::uint32_t>) {
    std::vector<std::byte> result;
    result.resize(4);
    std::memcpy(result.data(), &val, 4);
    return result;
}

std::vector<std::byte> Serialize(const CalculateId& id, formats::To<CalculateId>) {
    std::vector<std::byte> result;
    result.resize(12);
    std::uint32_t size = 8;
    std::memcpy(result.data(), &size, 4);

    std::uint32_t major = id.GetMajor();
    std::memcpy(result.data() + 4, &major, 4);
    std::uint32_t minor = id.GetMinor();
    std::memcpy(result.data() + 8, &minor, 4);
    return result;
}

std::vector<std::byte> Serialize(const Point& p, formats::To<Point>) {
    std::vector<std::byte> result;
    result.resize(sizeof(p));
    std::memcpy(result.data(), &p, sizeof(p));
    return result;
}

}