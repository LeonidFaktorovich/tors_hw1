#pragma once

#include <string>
#include <cstring>
#include <vector>

namespace formats {

template <typename T>
struct To {};

template <typename T>
std::vector<std::byte> Serialize(const T& val) {
    return Serialize(val, To<T>{});
}

std::vector<std::byte> Serialize(double val, formats::To<double>) {
    std::vector<std::byte> result;
    result.resize(sizeof(val));
    std::memcpy(result.data(), &val, sizeof(val));
    return result;
}

} // namesapce formats