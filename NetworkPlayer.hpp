#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include "Connection.hpp"
#include <vector>
#include <array>
#include <iostream>
# include "AnimationStateMachine.hpp"

// how many players in our game 
static const size_t PLAYER_NUM = 16;

// init positions/rotations for players
static const glm::vec3 playerInitPos = glm::vec3(1,0,0);
static const glm::vec3 playerInitPosDistance = glm::vec3(0.2,0.2,0);
static const glm::quat playerInitRot = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

// ------------ player class on the client side ------------ //
// (how/what client track the infos of players): client side send this to server
class Client_Player {
public:
    // info that will be sent to server
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 portal1_position;
    glm::quat portal1_rotation;
    glm::vec3 portal2_position;
    glm::quat portal2_rotation;
    uint8_t hit_id; // who I hit (0 means hit no one)
    AnimationState animState;
    unsigned int curFrame;

    Client_Player(){};
    Client_Player(
        glm::vec3 position_, glm::quat rotation_, glm::vec3 portal1_position_, 
        glm::quat portal1_rotation_, glm::vec3 portal2_position_, glm::quat ortal2_rotation_, uint8_t hit_id_,
        AnimationState animState_ , unsigned int curFrame_
    );
    // convert client side player's info into bytes (and send this to server)
    void convert_to_message(std::vector<unsigned char> & client_message);
    // read the message sent by the server, and put it in the client side player obj
    void read_from_message(
        const std::vector<unsigned char> & server_message, uint8_t & id, bool & gotHit, 
        AnimationState & animState_read, unsigned int & curFrame_read
    );

    static size_t Client_Player_mes_size;
};

// ------------- player class on the server side ---------------- //
// (how/what server track the infos of players): server side send this to client side for each player
class Server_Player {
public:
    // info that will be sent to client
    uint8_t id;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 portal1_position;
    glm::quat portal1_rotation;
    glm::vec3 portal2_position;
    glm::quat portal2_rotation;
    bool gotHit; // was hit by someone
    AnimationState animState;
    unsigned int curFrame;

    Server_Player();
    // convert server side player's info into bytes (and send this to client)
    void convert_to_message(std::vector<unsigned char> & server_message);
    // read the message sent by the client, and put it in the server side player obj
    void read_from_message(Connection * c, uint8_t & hit_id);

    static size_t Server_Player_mes_size;
    static std::array<bool, PLAYER_NUM> id_used;
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
union unsignedInt_as_byte
{
    unsigned int unsignedInt_value;
    unsigned char bytes_value[sizeof(unsigned int)];
};