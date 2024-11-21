#include <udp_server.hpp>

UdpServer::UdpServer(std::uint16_t port) {
    addrinfo hints;
    addrinfo* result = NULL;
    addrinfo* rp = NULL;
    std::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string port_str = std::to_string(port);
    int s = getaddrinfo(NULL, port_str.c_str(), &hints, &result);
    if (s != 0) {
        throw std::runtime_error("Error with getaddrinfo: " + std::string(gai_strerror(s)));
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        socket_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd_ == -1) {
            continue;
        }
        if (bind(socket_fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            
            break;
        }
        close(socket_fd_);
    }
}
