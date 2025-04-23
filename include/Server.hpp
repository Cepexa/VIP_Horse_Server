#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <thread>
#include <vector>

#include "InfoBase.hpp"
#include "ClassesList.hpp"
#include "BinaryProtocol.hpp"

class Server {
public:
    Server(uint16_t port);
    void start();

private:
    static int doAction(std::shared_ptr<RecordClass> rc, const std::string& sql_create);
    void acceptConnections();
    void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::vector<std::thread> client_threads_;
};

#endif // SERVER_HPP
