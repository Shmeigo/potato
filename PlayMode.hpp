#include "Mode.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "Connection.hpp"
#include "TextRenderer.hpp"
#include "NetworkPlayer.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	// input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	// --------- local game logics -------- //
	// my own info
	uint8_t my_id = 0; // 0 for unkonw (assigned by server)
    Scene::Transform *my_transform = nullptr;
    Scene::Camera *my_camera = nullptr;
    float mouse_x;
    float mouse_y;
	// scenes
	Scene scene;
	// gameplay related
	const glm::vec3 cameraOffset = glm::vec3(0.0f, -0.7f, .7f);;
	const glm::vec3 playerInitPos = glm::vec3(-7,-1,0);
	const glm::vec3 playerInitPosDistance = glm::vec3(0,-1.5f,0);
	const glm::quat playerInitRot = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec2 curVelocity = glm::vec3(0);
	float acceleration = 1.5f;
	float friction = 1.0f;
	bool ping;
	// font
	std::shared_ptr<TextRenderer> hintFont;
	std::shared_ptr<TextRenderer> messageFont;

	// ------- multiplayer game logics ---------- //
	// transforms of all players' model, including my model
	std::array <Scene::Transform *, PLAYER_NUM> players_transform{};
	//last message from server:
	std::vector<unsigned char> server_message;
	//connection to server:
	Client &client;

};