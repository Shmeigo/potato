#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include "Connection.hpp"
#include <vector>
#include <iostream>

// how many players in our game 
static const size_t PLAYER_NUM = 16;

// ------------ player class on the client side ------------ //
// (how/what client track the infos of players)
class Client_Player {
public:
    // info that will be sent to server
    glm::vec3 position;
    glm::quat rotation;

    Client_Player(){};
    Client_Player(glm::vec3 position_, glm::quat rotation_):position(position_),rotation(rotation_){};
    // convert client side player's info into bytes
    void convert_to_message(std::vector<unsigned char> & client_message);
    // read the message sent by the server, and put it in the client side player obj
    void read_from_message(const std::vector<unsigned char> & server_message, uint8_t & id);

    static size_t Client_Player_mes_size;
};

// ------------- player class on the server side ---------------- //
// (how/what server track the infos of players)
class Server_Player {
public:
    // info that will be sent to client
    uint8_t id;
    glm::vec3 position;
    glm::quat rotation;

    Server_Player();
    // convert server side player's info into bytes
    void convert_to_message(std::vector<unsigned char> & server_message);
    // read the message sent by the client, and put it in the server side player obj
    void read_from_message(Connection * c);

    static size_t Server_Player_mes_size;
};

// -------- unions for convertion ---------- //
union vec3_as_byte
{
	glm::vec3 vec3_value;
	unsigned char bytes_value[sizeof(glm::vec3)];
};
union quat_as_byte
{
	glm::quat quat_value;
	unsigned char bytes_value[sizeof(glm::quat)];
};
union sizet_as_byte
{
	size_t sizet_value;
	unsigned char bytes_value[sizeof(size_t)];
};