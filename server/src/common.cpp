#include <common.hpp>

ClientAddr ParseClientAddr(const sockaddr_in& addr) {
    ClientAddr client_addr;
    client_addr.port = addr.sin_port;

    client_addr.ip.resize(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &addr.sin_addr, client_addr.ip.data(), client_addr.ip.size());
    return client_addr;
}

ClientAddr ParseClientAddr(const sockaddr_storage& storage) {
    return ParseClientAddr(*reinterpret_cast<const sockaddr_in*>(&storage));
}