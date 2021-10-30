#include "NetworkPlayer.hpp"

size_t Client_Player::Client_Player_mes_size = sizeof(glm::vec3) + sizeof(glm::quat);
size_t Server_Player::Server_Player_mes_size = sizeof(uint8_t) + sizeof(glm::vec3) + sizeof(glm::quat);
std::array<bool, PLAYER_NUM> Server_Player::id_used = {false};

// -------------- client side ------------ //

void Client_Player::convert_to_message(std::vector<unsigned char> & client_message){
    assert(client_message.size() == 0);
    client_message.reserve(Client_Player_mes_size);

    // position
    vec3_as_byte position_bytes;
    position_bytes.vec3_value = position;
    for (size_t i =0; i < sizeof(glm::vec3); i++){
        client_message.emplace_back(position_bytes.bytes_value[i]);
    }
    // rotation
    quat_as_byte rotation_bytes;
    rotation_bytes.quat_value = rotation;
    for (size_t i =0; i < sizeof(glm::quat); i++){
        client_message.emplace_back(rotation_bytes.bytes_value[i]);
    }

    assert(client_message.size() == Client_Player_mes_size);
}

void Client_Player::read_from_message(const std::vector<unsigned char> & server_message, uint8_t & id){
    assert(server_message.size() == Server_Player::Server_Player_mes_size);
    // id
    id = (uint8_t)server_message[0];
    // position
    vec3_as_byte position_bytes;
    for (size_t j =0; j < sizeof(glm::vec3); j++){
        position_bytes.bytes_value[j] = server_message[j+1];
    }
    position = position_bytes.vec3_value;
    // rotation
    quat_as_byte rotation_bytes;
    for (size_t j =0; j < sizeof(glm::quat); j++){
        rotation_bytes.bytes_value[j] = server_message[j+1+sizeof(glm::vec3)];
    }
    rotation = rotation_bytes.quat_value;
}

// -------------- server side ------------ //

Server_Player::Server_Player() {
    // find the first non-used id
    id = 0;
    for (size_t i=0; i < id_used.size(); i++){
        if(!id_used[i]){
            id = (uint8_t)i + 1;
            id_used[i] = true;
            break;
        }
    }   
    assert(id != 0);
    position = glm::vec3(-7,-1,0);;
    rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}
    
// convert player's info into bytes, with form of string
void Server_Player::convert_to_message(std::vector<unsigned char> & server_message){
    size_t before = server_message.size();
    server_message.reserve(Server_Player_mes_size);

    // id
    server_message.emplace_back((unsigned char)id);
    // position
    vec3_as_byte position_bytes;
    position_bytes.vec3_value = position;
    for (size_t i =0; i < sizeof(position); i++){
        server_message.emplace_back (position_bytes.bytes_value[i]);
    }
    // rotation
    quat_as_byte rotation_bytes;
    rotation_bytes.quat_value = rotation;
    for (size_t i =0; i < sizeof(rotation); i++){
        server_message.emplace_back (rotation_bytes.bytes_value[i]);
    }

    assert(server_message.size() - before == Server_Player_mes_size);
}

void Server_Player::read_from_message(Connection * c){
    size_t client_mes_size = Client_Player::Client_Player_mes_size;
    while (c->recv_buffer.size() >= client_mes_size) {
        size_t index = 0;
        // expecting messages 'b' 
        char type = c->recv_buffer[0];
        if (type != 'b') {
            std::cout << " message of non-'b' type received from client!" << std::endl;
            //shut down client connection:
            c->close();
            return;
        }
        index += 1;
        // position
        vec3_as_byte position_mes;
        for (size_t j =0; j < sizeof(glm::vec3); j++){
            position_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        position = position_mes.vec3_value;
        index += sizeof(glm::vec3);
        // rotation
        quat_as_byte rotation_mes;
        for (size_t j =0; j < sizeof(glm::quat); j++){
            rotation_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        rotation = rotation_mes.quat_value;
        // erase bytes (remeber +1 for the 'b')
        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + client_mes_size + 1);
    }
}