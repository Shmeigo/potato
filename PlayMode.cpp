#include "PlayMode.hpp"
#include "LitColorTextureProgram.hpp"
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "Load.hpp"
#include "hex_dump.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint stage_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > stage_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("city.pnct"));
	stage_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > stage_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("city.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = stage_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = stage_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode(Client &client_) : client(client_),scene(*stage_scene) {

	// get the transforms of all players' models 
	for(uint8_t i =0; i < PLAYER_NUM; i++){
		for (auto &transform : scene.transforms) {
			if (transform.name == "Player" + std::to_string(i+1)) {
				players_transform[i] = &transform;
			}
			if (transform.name == "Player" + std::to_string(i+1) + "Portal1") {
				portal1_transform[i] = &transform;
			}
			if (transform.name == "Player" + std::to_string(i+1) + "Portal2") {
				portal2_transform[i] = &transform;
			}
		}
	}
	// disable other players' drawing, util recieve other players' info from server
	for (size_t i =0; i < players_transform.size(); i++){
		if (players_transform[i] == nullptr){
			throw std::runtime_error("!!! Does not have the model of player " + std::to_string(i) );
		} else if (portal1_transform[i] == nullptr){
			throw std::runtime_error("!!! Does not have the model of portal1 " + std::to_string(i) );
		} else if (portal2_transform[i] == nullptr){
			throw std::runtime_error("!!! Does not have the model of portal2  " + std::to_string(i) );
		} else {
			players_transform[i]->draw = false;
		}
	}

	//create a player transform:
	scene.transforms.emplace_back();
	my_transform = &scene.transforms.back();

	//create initial transforms for the portals:
	scene.transforms.emplace_back();
	p1_transform = &scene.transforms.back();

	scene.transforms.emplace_back();
	p2_transform = &scene.transforms.back();

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	my_camera = &scene.cameras.back();
	my_camera->fovy = glm::radians(60.0f);
	my_camera->near = 0.01f;

	//create camera controller to control the camera
	cameraController = new CameraController(my_camera, my_transform, glm::vec3(0.0f), 1.5f, 2.5f, 1.5f, 0.0f, 0.5f, 1.2f);

	//create player controller to control the player
	characterController = new CharacterController(my_transform, my_camera);

	// font
	hintFont = std::make_shared<TextRenderer>(data_path("OpenSans-B9K8.ttf"));
	messageFont = std::make_shared<TextRenderer>(data_path("SeratUltra-1GE24.ttf"));
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	// wasd
	// e to place portals
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			place.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			place.pressed = false;
			can_place = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	
	// mouse
	if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			mouse_x = evt.motion.xrel / float(window_size.y),
			mouse_y = -evt.motion.yrel / float(window_size.y);

			cameraController->AdjustCamera(glm::vec2(mouse_x, mouse_y));
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	// update my own transform locally
	{
		//combine inputs into a force:
		glm::vec2 force = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) force.x =-1.0f;
		if (!left.pressed && right.pressed) force.x = 1.0f;
		if (down.pressed && !up.pressed) force.y =-1.0f;
		if (!down.pressed && up.pressed) force.y = 1.0f;
		if (place.pressed && can_place) {
			if (place_p1) {
				p1_transform->position = my_transform->position;
				p1_transform->rotation = my_transform->rotation;
			} else {
				p2_transform->position = my_transform->position;
				p2_transform->rotation = my_transform->rotation;
				both_placed = true;
			}
			can_teleport = false;
			can_place = false;
			place_p1 = !place_p1;
		}

		characterController->UpdateCharacter(force, elapsed);

		// check if I stepped into a portal
		if (glm::distance(my_transform->position, p1_transform->position) < 0.5f && 
					can_teleport && both_placed) {
			can_teleport = false;
			my_transform->position = p2_transform->position;
		} else if (glm::distance(my_transform->position, p2_transform->position) < 0.5f && 
					can_teleport && both_placed) {
			can_teleport = false;
			my_transform->position = p1_transform->position;
		} else if (glm::distance(my_transform->position, p1_transform->position) > 0.5f && 
					glm::distance(my_transform->position, p2_transform->position) > 0.5f) {
			can_teleport = true;
		}

		// update my model's transform, if i know which model is mine
		if(my_id!=0){
			players_transform[my_id-1]->position = my_transform->position;
			players_transform[my_id-1]->rotation = my_transform->rotation;
			portal1_transform[my_id-1]->position = p1_transform->position;
			portal1_transform[my_id-1]->rotation = p1_transform->rotation;
			portal2_transform[my_id-1]->position = p2_transform->position;
			portal2_transform[my_id-1]->rotation = p2_transform->rotation;
		}
	}

	cameraController->UpdateCamera();

	// sending my info to server:
	if (left.pressed || right.pressed || down.pressed || up.pressed || mouse_x!=0 || place.pressed || !can_teleport) {
		// convert info to msg
		Client_Player myself(my_transform->position, my_transform->rotation);
		std::vector<unsigned char> client_message;
		myself.convert_to_message(client_message);
		// send msg
		Connection & c = client.connections.back();
		c.send('b');
		c.send_buffer.insert(c.send_buffer.end(), client_message.begin(), client_message.end());
	}

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} 
		else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} 
		else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
			// descriptor: 'm' + size_of_message
			size_t descriptor_size = 1 + sizeof(size_t);
			
			while (c->recv_buffer.size() >= descriptor_size) {
				//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
				}
				// get size
				sizet_as_byte sizet_bytes;
				for(size_t i=0; i < sizeof(size_t); i++){
					sizet_bytes.bytes_value[i] = c->recv_buffer[1+i];
				}
				size_t size = sizet_bytes.sizet_value;
				if (c->recv_buffer.size() < descriptor_size + size) break; //if whole message isn't here, can't process
				//whole message *is* here, so set current server message:
				server_message = std::vector<unsigned char>(c->recv_buffer.begin() + descriptor_size, c->recv_buffer.begin() + descriptor_size + size);
				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + descriptor_size + size);
			}
		}
	}, 0.0);

	// update local state according to the server's message
	{
		assert(server_message.size() > 0);
		// read the public stuff before offset. These are infos same for all players.
		ping = (bool)server_message[0];
		size_t i_offset = 1; // message after this contains all players' individual infos

		// read player's info one by one
		for(size_t i =i_offset; i < server_message.size(); ){

			// get content of this player
			std::vector<unsigned char> content(server_message.begin()+i, server_message.begin()+i+Server_Player::Server_Player_mes_size);
			// ---------- read content ----------- //
			Client_Player client_player;
			uint8_t id;
			client_player.read_from_message(content, id);

			// --------- process info ---------- //
			// is this my info ? (server will put my own info at first)
			if(i == i_offset){
				// if unkonwn before
				if(my_id == 0){
					my_id = id;
					// set my init position and rotation accroding to my id
					my_transform->position = playerInitPos + playerInitPosDistance * (float)(id-1);
					my_transform->rotation = playerInitRot;
					p1_transform->position = portalInitPos;
					// p1_transform->rotation = playerInitRot;
					p2_transform->position = portalInitPos;
					// p2_transform->rotation = playerInitRot;
					// enable my own model's drawing (delete if want to disable)
					players_transform[id-1]->draw = true;
				}
			}
			// other players' info, update their models' transform
			else{
				players_transform[id-1]->draw = true;

				players_transform[id-1]->position = client_player.position;
				// portal1_transform[id-1]->position = client_player.position;
				// portal2_transform[id-1]->position = client_player.position;

				players_transform[id-1]->rotation = client_player.rotation;
				// portal1_transform[id-1]->rotation = client_player.rotation;
				// portal2_transform[id-1]->rotation = client_player.rotation;
			}

			// move to next player's info
			i += Server_Player::Server_Player_mes_size;
		}

		// game logic update here
	}

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	//update camera aspect ratio for drawable:
	my_camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*my_camera);

	// text
	hintFont->draw("Your Are Player " + std::to_string(my_id), 20.0f, 550.0f, glm::vec2(0.2,0.25), glm::vec3(0.2, 0.8f, 0.2f));
	if(ping)
		messageFont->draw("*****", 20.0f, 500.0f, glm::vec2(0.2,0.25), glm::vec3(0.9, 0.8f, 0.2f));
	else
		messageFont->draw("-----", 20.0f, 500.0f, glm::vec2(0.2,0.25), glm::vec3(0.9, 0.8f, 0.2f));


	GL_ERRORS();
}
