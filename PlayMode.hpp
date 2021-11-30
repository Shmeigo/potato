#include "Mode.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "Connection.hpp"
#include "TextRenderer.hpp"
#include "NetworkPlayer.hpp"
#include "CameraController.hpp"
#include "CharacterController.hpp"
#include "CollisionSystem.hpp"
#include "AnimationStateMachine.hpp"
#include "TwoDRenderer.hpp"

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
	} left, right, down, up, place, attack, block;

	std::vector<AnimationStateMachine> animation_machines;
	float frametime = 0.f;

	// --------- local game logics -------- //
	// my own info
	uint8_t my_id = 0; // 0 for unkonw (assigned by server)
    Scene::Transform *my_transform = nullptr;
    Scene::Transform *p1_transform = nullptr;
    Scene::Transform *p2_transform = nullptr;
    Scene::Camera *my_camera = nullptr;
    float mouse_x;
    float mouse_y;
	bool place_p1 = true;
	bool can_place = true;
	bool can_teleport = true;
	bool both_placed = false;
	bool pre_move = false;

	// scenes
	Scene scene;

	// Camera Controller
	CameraController * cameraController;

	// Player Controller
	CharacterController * characterController;

	// Collision System
	CollisionSystem* collisionSystem;

	// gameplay related
	const float attackDegree = 90.0f;
	const float attackRadius = 2.0f;
	const glm::vec3 portalInitPos = glm::vec3(-7,-1,-3);
	uint8_t hit_id = 0; // who I hit (0 means hit no one)
	bool hitLastTime = false;

	// attack
	const float hitCD = 1.5f;
	const float hitWindup = 0.5f;
	float hitTimer = 0.0f;

	// block
	const float blockCD = 0.8f;
	const float blockZone = 0.3f;
	float blockTimer = 0.0f;

	// stun
	const float stunCD = 1.1f;
	float stunTimer = 0.0f;

	// health
	const float Maxhealth = 100.0f;
	const float damage = 25.0f;
	float health = 100.0f;
	
	bool ping;

	// font
	std::shared_ptr<TextRenderer> hintFont;
	std::shared_ptr<TextRenderer> messageFont;

	// sprite
	std::shared_ptr<TwoDRenderer> heart;
	std::shared_ptr<TwoDRenderer> sword;

	// ------- multiplayer game logics ---------- //
	// transforms of all players' model, including my model
	std::array <Scene::Transform *, PLAYER_NUM> players_transform{};
	// transforms of all players' portal models, including my models
	std::array <Scene::Transform *, PLAYER_NUM> portal1_transform{};
	std::array <Scene::Transform *, PLAYER_NUM> portal2_transform{};
	//last message from server:
	std::vector<unsigned char> server_message;
	//connection to server:
	Client &client;

	// sounds 
	const float SONG_REPEAT_TIME = 100.0f;
	float song_timer = 0.0f;
	std::shared_ptr<Sound::PlayingSample> background_music;
	std::shared_ptr<Sound::PlayingSample> combat_music;
	bool current_background = true;
	bool start = true;

	bool is_in_combat = false;
	const float MAX_COMBAT_TIME = 10.0f;
	float combat_timer = 0.0f;
	


};
