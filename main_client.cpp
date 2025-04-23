#include "Client.hpp"

int main() {
    sleep(2);
    Client client("127.0.0.1", 12345);

    uint64_t count_req = 1;
    BinaryProtocol::Packet p(BinaryProtocol::CommandType::SQLR,count_req++);
    using tags = BinaryProtocol::SQL_Tags;
    p.addData(tags::SELECT, "*");
    p.addData(tags::FROM, "r11recordtest2");
    p.addData(tags::WHERE, "id = 1");


    BinaryProtocol::Packet pr = client.sendCommand(p);

    //client.sendCommand(BinaryProtocol::CommandType::CREATE_TABLE, "test_create_table (id INTEGER NOT NULL PRIMARY KEY);");
    //client.sendCommand(BinaryProtocol::CommandType::ADD_COLUMN, "test_create_table (id INTEGER NOT NULL PRIMARY KEY);");
    return 0;
}
