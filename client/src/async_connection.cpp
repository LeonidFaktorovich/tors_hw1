#include <async_connection.hpp>
#include <mutex>
#include <stdexcept>
#include <thread>

TcpClient::TcpClient(const std::string& domain, std::uint16_t port) {
    addrinfo hints;
    addrinfo* res;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    std::string service = std::to_string(port);
    if (getaddrinfo(domain.c_str(), service.c_str(), &hints, &res) != 0) {
        throw std::runtime_error("Error with getaddrinfo");
    }

    socket_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd_ < 0) {
        throw std::runtime_error("Error with socket");
    }

    if (connect(socket_fd_, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        close(socket_fd_);
        throw std::runtime_error("Error with connect");
    }
    freeaddrinfo(res);
}

void TcpClient::EnableTcpKeepalive() {
    int optval = 1;
    socklen_t optlen = sizeof(optval);

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        throw std::runtime_error("Error with enable tcp keepalive");
    }

}

TcpClient::~TcpClient() {
    close(socket_fd_);
}


AsyncConnection::AsyncConnection(const std::string& domain, std::uint16_t port) {
    worker_ = std::thread([domain, port, this] {
        while (!IsClosed()) {
            try {
                TcpClient client(domain, port);
                client.EnableTcpKeepalive();
                while (!IsClosed()) {
                    auto request = EnqueueRequest();
                    if (!request.has_value()) {
                        return;
                    }
                    client.Write(request->id);
                    std::uint32_t points_count = request->points.size();
                    client.Write(points_count);
                    for (auto point : request->points) {
                        client.Write(point);
                    }

                    auto result_id = CalculateId::ParseFromConnection(client);
                    double result;
                    client.Read(result);

                    std::lock_guard guard(results_mutex_);
                    results_.push_back(Result{.id = result_id, .result = result});
                }
            } catch(const std::exception& e) {
                std::cout << "Error: " << e.what();
            }
        }
    });
}

AsyncConnection::~AsyncConnection() {
    worker_.join();
}

void AsyncConnection::SendRequest(Request r) {
    std::lock_guard guard(requests_mutex_);
    requests_.push_back(std::move(r));
    if (requests_.size() == 1) {
        cv_.notify_one();
    }
}

std::vector<Result> AsyncConnection::GetResults() {
    std::vector<Result> results;
    std::lock_guard guard(results_mutex_);
    std::swap(results_, results);
    return results;
}

std::optional<Request> AsyncConnection::EnqueueRequest() {
    std::unique_lock lock(requests_mutex_);
    if (is_closed_) {
        return std::nullopt;
    }

    if (!requests_.empty()) {
        auto request = std::move(requests_.back());
        requests_.pop_back();
        return request;
    }

    cv_.wait(lock, [this] {
        return !requests_.empty() || is_closed_;
    });

    if (!requests_.empty()) {
        auto request = std::move(requests_.back());
        requests_.pop_back();
        return request;
    }

    return std::nullopt;
}

void AsyncConnection::Close() {
    std::lock_guard guard(requests_mutex_);
    is_closed_ = true;
    cv_.notify_one();
    std::cout << "Close connection" << std::endl;
}

bool AsyncConnection::IsClosed() {
    std::lock_guard guard(requests_mutex_);
    return is_closed_;
}