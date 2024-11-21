#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

struct ClientAddr {
    std::string ip;
    std::uint16_t port;
};

ClientAddr ParseClientAddr(const sockaddr_in& addr);

ClientAddr ParseClientAddr(const sockaddr_storage& storage);