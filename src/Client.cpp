#include "Client.hpp"
#include <iostream>

Client::Client(const std::string& host, uint16_t port)
    : socket_(io_context_) {
    boost::asio::ip::tcp::resolver resolver(io_context_);
    boost::asio::connect(socket_, resolver.resolve(host, std::to_string(port)));
}

BinaryProtocol::Packet Client::sendCommand(const BinaryProtocol::Packet& packet) {
    try {
        auto binaryData = packet.toBinary();

        // Отправляем данные
        boost::asio::write(socket_, boost::asio::buffer(binaryData));

        std::vector<uint8_t> buffer(1024);
        boost::system::error_code error;

        // Читаем ответ сервера
        size_t len = socket_.read_some(boost::asio::buffer(buffer), error);

        if (error == boost::asio::error::eof) {
            std::cout << "Сервер закрыл соединение." << std::endl;
            throw boost::system::system_error(error);
        } else if (error) {
            throw boost::system::system_error(error);
        }

        BinaryProtocol::Packet response = BinaryProtocol::Packet::fromBinary({buffer.begin(), buffer.begin() + len});
        
        switch (response.header.command)
        {
        case BinaryProtocol::CommandType::OK:
            std::cout << "Ответ от сервера: OK" << std::endl;
            break;            
        case BinaryProtocol::CommandType::SQLR:
            std::cout << "Ответ от сервера: SQLR" << std::endl;
            break;            
        default:
            std::cout << "Ответ от сервера: " << static_cast<int>(response.header.command) << std::endl;
            break;
        }


        // Корректно закрываем соединение
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();
        return response;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }
}

