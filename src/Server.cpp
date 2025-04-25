#include "Server.hpp"

#include <iostream>

using command = BinaryProtocol::CommandType;

Server::Server(uint16_t port)
    : acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}

int Server::doAction(std::shared_ptr<RecordClass> rc, const std::string& sql_create) {
    int count = 0;
    std::string nameRC = rc->getName();
    InfoBase::getInstance().registerRecord(rc);
    InfoBase::getInstance().execute_query("CREATE TABLE IF NOT EXISTS " + nameRC + sql_create);
    if (nameRC[0] == 'r')
    {
        pqxx::result res = InfoBase::getInstance().execute_query(
            "SELECT COALESCE(MAX(id), 0) FROM " + rc->getName() + ";");
        if (!res.empty()) { count = res[0][0].as<int>(); }
    }
#ifdef DEBUG
    std::cout << "initStatic table name: "<<rc->getName()<<std::endl; 
#endif
    return count;
}

void Server::start() {
    
    std::cout << "Сервер запущен на порту " << acceptor_.local_endpoint().port() << std::endl;
    
    for (const auto& initFunc : initStaticFunctions) {
        initFunc(doAction);
    }
    InfoBase::getInstance().syncDatabase();
    
    acceptConnections();
    io_context_.run();
}

void Server::acceptConnections() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
    acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
        if (!error) {
            client_threads_.emplace_back(&Server::handleClient, this, socket);
        }
        acceptConnections(); // Ждем следующее соединение
    });
}

void Server::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    try {
        boost::asio::ip::tcp::endpoint remote_ep = socket->remote_endpoint();
        std::string client_id = remote_ep.address().to_string() + ":" + std::to_string(remote_ep.port());
        std::cout << "Подключен новый клиент: " << client_id << std::endl;
        while (true) {
            std::vector<uint8_t> buffer(1024);
            boost::system::error_code error;

            // Ожидаем получение данных от клиента
            size_t len = socket->read_some(boost::asio::buffer(buffer), error);

            if (error == boost::asio::error::eof) {
                std::cout << "Клиент отключился." << std::endl;
                break;
            } else if (error) {
                throw boost::system::system_error(error);
            }

            // Разбираем пакет
            BinaryProtocol::PacketRequest packet = BinaryProtocol::PacketRequest::fromBinary({buffer.begin(), buffer.begin() + len});
            std::cout << "Получена команда: " << static_cast<int>(packet.header.command) << std::endl;
            pqxx::result res;
            command response_cmd;
            // Обработка команды
            switch (packet.header.command)
            {
            case command::SQL:
                res = InfoBase::getInstance().execute_query(packet.getQuery());
                response_cmd = res.empty() ? command::EMPTY : command::OK;
                break;            
            default:
                response_cmd = command::ERROR;
                break;
            }

            // Создаём ответный пакет
            BinaryProtocol::PacketResponse response(response_cmd, packet.header.request_id);
            if (!res.empty()) 
            {
                for (const auto& field : GetRecordClassFunctions[0]()->getFields()) {
                    const std::string& name = field.getName();
                    if(!field.isPrimaryKey())
                    {
                        Variant value = convertFieldVariant(field, res[0][name]);
                        std::string binary;
                        std::memcpy(binary.data(), VariantToCStr(value), sizeof(value));
                        response.addNameValue(name, binary);
                    }
                }
            }

            auto binaryResponse = response.toBinary();

            // Отправляем ответ клиенту
            boost::asio::write(*socket, boost::asio::buffer(binaryResponse));
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка в клиентском потоке: " << e.what() << std::endl;
    }
}

