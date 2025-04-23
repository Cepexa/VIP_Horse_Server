#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>
#include "BinaryProtocol.hpp"

class Client {
public:
    Client(const std::string& host, uint16_t port);
    BinaryProtocol::Packet sendCommand(const BinaryProtocol::Packet& packet);

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
};

#endif // CLIENT_HPP
