#include "NetworkPlayer.hpp"

size_t Client_Player::Client_Player_mes_size = ( sizeof(glm::vec3) + sizeof(glm::quat) ) * 3 + 1 + 1 + sizeof(unsigned int);
size_t Server_Player::Server_Player_mes_size = sizeof(uint8_t) + ( sizeof(glm::vec3) + sizeof(glm::quat) ) * 3 + sizeof(bool) + 1 + sizeof(unsigned int);
std::array<bool, PLAYER_NUM> Server_Player::id_used = {false};

// ------------------------------ client side -------------------------- //

Client_Player::Client_Player(
    glm::vec3 position_, glm::quat rotation_, glm::vec3 portal1_position_, 
    glm::quat portal1_rotation_, glm::vec3 portal2_position_, glm::quat portal2_rotation_, uint8_t hit_id_,
    AnimationState animState_ , unsigned int curFrame_
):
position(position_),rotation(rotation_), portal1_position(portal1_position_), portal1_rotation(portal1_rotation_),
portal2_position(portal2_position_), portal2_rotation(portal2_rotation_), hit_id(hit_id_),
animState(animState_), curFrame(curFrame_)
{};

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
    // portal1 position
    vec3_as_byte portal1_position_bytes;
    portal1_position_bytes.vec3_value = portal1_position;
    for (size_t i =0; i < sizeof(glm::vec3); i++){
        client_message.emplace_back(portal1_position_bytes.bytes_value[i]);
    }
    // portal1 rotation
    quat_as_byte portal1_rotation_bytes;
    portal1_rotation_bytes.quat_value = portal1_rotation;
    for (size_t i =0; i < sizeof(glm::quat); i++){
        client_message.emplace_back(portal1_rotation_bytes.bytes_value[i]);
    }
    // portal2 position
    vec3_as_byte portal2_position_bytes;
    portal2_position_bytes.vec3_value = portal2_position;
    for (size_t i =0; i < sizeof(glm::vec3); i++){
        client_message.emplace_back(portal2_position_bytes.bytes_value[i]);
    }
    // portal2 rotation
    quat_as_byte portal2_rotation_bytes;
    portal2_rotation_bytes.quat_value = portal2_rotation;
    for (size_t i =0; i < sizeof(glm::quat); i++){
        client_message.emplace_back(portal2_rotation_bytes.bytes_value[i]);
    }
    // hit id
    client_message.emplace_back((unsigned char) hit_id );
    // anim state
    client_message.emplace_back((unsigned char) animState );
    // current frame
    unsignedInt_as_byte curFrame_bytes;
    curFrame_bytes.unsignedInt_value = curFrame;
    for (size_t i =0; i < sizeof(unsigned int); i++){
        client_message.emplace_back(curFrame_bytes.bytes_value[i]);
    }

    // -----------
    assert(client_message.size() == Client_Player_mes_size);
}

void Client_Player::read_from_message(const std::vector<unsigned char> & server_message, uint8_t & id, bool & gotHit, 
    AnimationState & animState_read, unsigned int & curFrame_read)
{
    assert(server_message.size() == Server_Player::Server_Player_mes_size);
    // id
    id = (uint8_t)server_message[0];
    size_t index = 1;
    // position
    vec3_as_byte position_bytes;
    for (size_t j =0; j < sizeof(glm::vec3); j++){
        position_bytes.bytes_value[j] = server_message[j + index];
    }
    position = position_bytes.vec3_value;
    index += sizeof(glm::vec3);
    // rotation
    quat_as_byte rotation_bytes;
    for (size_t j =0; j < sizeof(glm::quat); j++){
        rotation_bytes.bytes_value[j] = server_message[j + index];
    }
    rotation = rotation_bytes.quat_value;
    index += sizeof(glm::quat);
    // portal1_position
    vec3_as_byte portal1_position_bytes;
    for (size_t j =0; j < sizeof(glm::vec3); j++){
        portal1_position_bytes.bytes_value[j] = server_message[j + index];
    }
    portal1_position = portal1_position_bytes.vec3_value;
    index += sizeof(glm::vec3);
    // portal1_rotation
    quat_as_byte portal1_rotation_bytes;
    for (size_t j =0; j < sizeof(glm::quat); j++){
        portal1_rotation_bytes.bytes_value[j] = server_message[j + index];
    }
    portal1_rotation = portal1_rotation_bytes.quat_value;
    index += sizeof(glm::quat);
    // portal2_position
    vec3_as_byte portal2_position_bytes;
    for (size_t j =0; j < sizeof(glm::vec3); j++){
        portal2_position_bytes.bytes_value[j] = server_message[j + index];
    }
    portal2_position = portal2_position_bytes.vec3_value;
    index += sizeof(glm::vec3);
    // portal2_rotation
    quat_as_byte portal2_rotation_bytes;
    for (size_t j =0; j < sizeof(glm::quat); j++){
        portal2_rotation_bytes.bytes_value[j] = server_message[j + index];
    }
    portal2_rotation = portal2_rotation_bytes.quat_value;
    index += sizeof(glm::quat);
    // got hit
    gotHit = (bool) server_message[index];
    index += sizeof(bool);
    // anim state
    animState_read = (AnimationState) server_message[index];
    index += 1;
    // cur frame
    unsignedInt_as_byte curFrame_bytes;
    for (size_t j =0; j < sizeof(unsigned int); j++){
        curFrame_bytes.bytes_value[j] = server_message[j + index];
    }
    curFrame_read = curFrame_bytes.unsignedInt_value;
    index += sizeof(unsigned int);
}

// ----------------------------- server side --------------------------- //

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
    position = playerInitPos + playerInitPosDistance * (float)(id-1);
	rotation = playerInitRot;
}
    
// convert player's info into bytes
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
    // portal1_position
    vec3_as_byte portal1_position_bytes;
    portal1_position_bytes.vec3_value = portal1_position;
    for (size_t i =0; i < sizeof(portal1_position); i++){
        server_message.emplace_back (portal1_position_bytes.bytes_value[i]);
    }
    // portal1_rotation
    quat_as_byte portal1_rotation_bytes;
    portal1_rotation_bytes.quat_value = portal1_rotation;
    for (size_t i =0; i < sizeof(portal1_rotation); i++){
        server_message.emplace_back (portal1_rotation_bytes.bytes_value[i]);
    }
    // portal2_position
    vec3_as_byte portal2_position_bytes;
    portal2_position_bytes.vec3_value = portal2_position;
    for (size_t i =0; i < sizeof(portal2_position); i++){
        server_message.emplace_back (portal2_position_bytes.bytes_value[i]);
    }
    // portal2_rotation
    quat_as_byte portal2_rotation_bytes;
    portal2_rotation_bytes.quat_value = portal2_rotation;
    for (size_t i =0; i < sizeof(portal2_rotation); i++){
        server_message.emplace_back (portal2_rotation_bytes.bytes_value[i]);
    }
    // got hit
    server_message.emplace_back((unsigned char) gotHit);
    // anim state
    server_message.emplace_back((AnimationState) animState);
    // current frame
    unsignedInt_as_byte curFrame_bytes;
    curFrame_bytes.unsignedInt_value = curFrame;
    for (size_t i =0; i < sizeof(unsigned int); i++){
        server_message.emplace_back(curFrame_bytes.bytes_value[i]);
    }

    assert(server_message.size() - before == Server_Player_mes_size);
}

void Server_Player::read_from_message(Connection * c, uint8_t & hit_id){
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
        index += sizeof(glm::quat);
        // portal 1 position
        vec3_as_byte portal1_position_mes;
        for (size_t j =0; j < sizeof(glm::vec3); j++){
            portal1_position_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        portal1_position = portal1_position_mes.vec3_value;
        index += sizeof(glm::vec3);
        // portal 1 rotation
        quat_as_byte portal1_rotation_mes;
        for (size_t j =0; j < sizeof(glm::quat); j++){
            portal1_rotation_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        portal1_rotation = portal1_rotation_mes.quat_value;
        index += sizeof(glm::quat);
        // portal 2 position
        vec3_as_byte portal2_position_mes;
        for (size_t j =0; j < sizeof(glm::vec3); j++){
            portal2_position_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        portal2_position = portal2_position_mes.vec3_value;
        index += sizeof(glm::vec3);
        // portal 2 rotation
        quat_as_byte portal2_rotation_mes;
        for (size_t j =0; j < sizeof(glm::quat); j++){
            portal2_rotation_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        portal2_rotation = portal2_rotation_mes.quat_value;
        index += sizeof(glm::quat);
        // hit id
        hit_id = (uint8_t)c->recv_buffer[index];
        index += 1;
        // anim state
        animState = (AnimationState)c->recv_buffer[index];
        index += 1;
        // cur frame
        unsignedInt_as_byte curFrame_mes;
        for (size_t j =0; j < sizeof(unsigned int); j++){
            curFrame_mes.bytes_value[j] = c->recv_buffer[index+j];
        }
        curFrame = curFrame_mes.unsignedInt_value;
        index += sizeof(unsigned int);

        // erase bytes (remeber +1 for the 'b')
        c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + client_mes_size + 1);
    }
}