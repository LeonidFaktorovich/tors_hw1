#pragma once

#include <string>
#include <cstring>
#include <vector>

class CalculateId;
class Point;

namespace formats {

template <typename T>
struct To {};

template <typename T>
std::vector<std::byte> Serialize(const T& val) {
    return Serialize(val, formats::To<T>{});
}

std::vector<std::byte> Serialize(std::uint32_t val, formats::To<std::uint32_t>);
std::vector<std::byte> Serialize(const CalculateId& id, formats::To<CalculateId>);
std::vector<std::byte> Serialize(const Point& p, formats::To<Point>);

}