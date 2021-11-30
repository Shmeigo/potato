#include "PlayMode.hpp"
#include "LitColorTextureProgram.hpp"
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "Load.hpp"
#include "hex_dump.hpp"
#include "data_path.hpp"
#include "Sound.hpp"


#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

#define BACKGROUND_VOL 0.3f
#define COMBAT_VOL 0.5f
#define FX_VOL 0.4f

GLuint stage_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > stage_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("field.pnct"));
	stage_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > stage_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("field.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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

Load< Sound::Sample > weird_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Weird.opus"));
});

Load< Sound::Sample > weirder_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Weirder.opus"));
});

Load< Sound::Sample > grime_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Grime.opus"));
});

Load< Sound::Sample > swing_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Swing.opus"));
});

Load< Sound::Sample > hit_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Hit.opus"));
});

Load< Sound::Sample > block_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Block.opus"));
});

Load< Sound::Sample > damage_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Kill.opus"));
});

Load< Sound::Sample > step_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Step.opus"));
});



PlayMode::PlayMode(Client &client_) : scene(*stage_scene), client(client_) {
	for (int i = 0; i < PLAYER_NUM; i++) {
		animation_machines.emplace_back();
	}

	//create a player transform:
	scene.transforms.emplace_back();
	my_transform = &scene.transforms.back();
	my_transform->position = playerInitPos;
	my_transform->rotation = playerInitRot;

	//create initial transforms for the portals:
	scene.transforms.emplace_back();
	p1_transform = &scene.transforms.back();

	scene.transforms.emplace_back();
	p2_transform = &scene.transforms.back();

	//create a player camera
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	my_camera = &scene.cameras.back();
	my_camera->fovy = glm::radians(60.0f);
	my_camera->near = 0.01f;

	//create camera controller to control the camera
	cameraController = new CameraController(my_camera, my_transform, glm::vec3(0.0f), 1.5f, 2.5f, 1.5f, 0.0f, 0.5f, 1.2f);

	//create player controller to control the player
	characterController = new CharacterController(my_transform, my_camera);

	//create collision system
	collisionSystem = new CollisionSystem();

	// get the transforms of all players' models and portals
	for(uint8_t i =0; i < PLAYER_NUM; i++){
		for (auto &transform : scene.transforms) {
			if (transform.name == "Player" + std::to_string(i+1)) {
				players_transform[i] = &transform;
				collisionSystem->AddElement(new CollisionSystem::Collidable(collisionSystem, &transform, 0.15f));
				// add skeletal
				if(true)	// can only add one skelatal, adding more will break the sahder
				{
					std::cout <<"adding skelatal \n";
					scene.skeletals.emplace_back(&transform);
				}
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
			portal1_transform[i]->draw = false;
			portal2_transform[i]->draw = false;
		}
	}

	// font
	hintFont = std::make_shared<TextRenderer>(data_path("OpenSans-B9K8.ttf"));
	messageFont = std::make_shared<TextRenderer>(data_path("SeratUltra-1GE24.ttf"));
	heart = std::make_shared<TwoDRenderer>(data_path("heart.png"));
	sword = std::make_shared<TwoDRenderer>(data_path("sword.png"));

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	// wasd
	// e to place portals
	// left mouse button to attack
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = true;
			//if(my_id != 0) animation_machines[my_id-1].set_state(RUN);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = true;
			//if(my_id != 0) animation_machines[my_id-1].set_state(RUN);
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = true;
			//if(my_id != 0) animation_machines[my_id-1].set_state(RUN);
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = true;
			//if(my_id != 0) animation_machines[my_id-1].set_state(RUN);
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			place.pressed = true;
			return true;
		} /*else if (evt.key.keysym.sym == SDLK_f) {
			if(my_id != 0) animation_machines[my_id-1].set_state(HIT_1);
			return true;
		} else if (evt.key.keysym.sym == SDLK_v) {
			if(my_id != 0) animation_machines[my_id-1].set_state(BLOCK);
			return true;
		} */
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			//if(my_id != 0) animation_machines[my_id-1].set_state(IDLE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			//if(my_id != 0) animation_machines[my_id-1].set_state(IDLE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			//if(my_id != 0) animation_machines[my_id-1].set_state(IDLE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			//if(my_id != 0) animation_machines[my_id-1].set_state(IDLE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			place.pressed = false;
			can_place = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			
		}
		if (evt.button.button == SDL_BUTTON_LEFT) {
			attack.pressed = true;
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_RIGHT) {
			block.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (evt.button.button == SDL_BUTTON_LEFT) {
			attack.pressed = false;
			return true;
		}
		else if (evt.button.button == SDL_BUTTON_RIGHT) {
			block.pressed = false;
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
	Sound::listener.set_position_right(my_transform->position, my_transform->rotation * glm::vec3(1.0f, 0.0f, 0.0f));

	frametime += elapsed;
	if (frametime >= 0.03f && my_id != 0) {
		// std::cerr << "Frametime update starts\n";
		frametime = 0;
		// time to update animation to new frame
		for (auto& machine : animation_machines) {
			machine.update();
		}

		unsigned times_run_start = animation_machines[0].times[RUN].first;
		unsigned times_hit_start = animation_machines[0].times[HIT_1].first;

		for (int i = 0; i < PLAYER_NUM; i++) {
			bool play_step = ((animation_machines[i].current_frame - times_run_start) == 5) || ((animation_machines[i].current_frame - times_run_start) == 15);
			bool play_swing = (animation_machines[i].current_frame - times_hit_start) == 5;

			switch (animation_machines[i].current_state) {
				case RUN:
				if (play_step) Sound::play_3D(*step_sample, FX_VOL, players_transform[i]->position, 1.0f);
				break;

				case HIT_1:
				if (play_swing) Sound::play_3D(*swing_sample, FX_VOL, players_transform[i]->position, 1.0f);
				break;

				default:
				break;
			}
		}
		// std::cerr << "Animation machine update done\n";
		// there is only one skeletal right now: the player
		// extend this to other skeletals by calling update_nodes() with the frame you want to update to
		// YOU NEED TO CALL update_nodes() for the skeletal to update
		
		//scene.skeletals.front().update_nodes(player_animation_machine.current_frame);
		// if(my_id !=0 && my_id-1 < scene.skeletals.size()){
		// 	scene.skeletals[my_id-1].update_nodes(player_animation_machine.current_frame);
		// }

		int sk_id = 0;
		for (auto sk = scene.skeletals.begin(); sk != scene.skeletals.end(); sk++) {
			// std::cerr << "skeletal update start\n";
			sk->update_nodes(animation_machines[sk_id].current_frame);
			sk_id++;
			// std::cerr << "skeletal update end\n";
		}
		// std::cerr << "Frametime update ends\n";
	}
	combat_timer -= elapsed;
	song_timer += elapsed;

	if (is_in_combat && combat_timer <= 0) {
		is_in_combat = false;
		combat_music->stop(1.0f);
		song_timer = SONG_REPEAT_TIME + 1.0f;
	}

	if ((!is_in_combat) && (start || song_timer >= SONG_REPEAT_TIME)) {
		start = false;
		song_timer = 0;
		current_background = !current_background;
		if (current_background) {
			background_music = Sound::play(*weird_sample, BACKGROUND_VOL);
		}
		else {
			background_music = Sound::play(*weirder_sample, BACKGROUND_VOL);
		}
	}

	

	// update my own transform locally
	if (my_id != 0) {

		if (hitTimer <= 0.0f && blockTimer <= 0.0f && stunTimer <= 0.0f) {
			//combine inputs into a force:
			glm::vec2 force = glm::vec2(0.0f);
			if (left.pressed && !right.pressed) force.x = -1.0f;
			if (!left.pressed && right.pressed) force.x = 1.0f;
			if (down.pressed && !up.pressed) force.y = -1.0f;
			if (!down.pressed && up.pressed) force.y = 1.0f;

			// update player movement
			characterController->UpdateCharacter(force, elapsed);

			// run animation
			bool cur_move = (force != glm::vec2(0.0f));
			if (cur_move && !pre_move) {
				animation_machines[my_id - 1].set_state(RUN);
			}
			if (!cur_move && pre_move){
				animation_machines[my_id - 1].set_state(IDLE);
			}
			pre_move = cur_move;


			// place portals
			if (place.pressed && can_place) {
				if (place_p1) {
					p1_transform->position = my_transform->position;
					p1_transform->rotation = my_transform->rotation;
				}
				else {
					p2_transform->position = my_transform->position;
					p2_transform->rotation = my_transform->rotation;
					both_placed = true;
				}
				can_teleport = false;
				can_place = false;
				place_p1 = !place_p1;
			}

			// check if I stepped into a portal
			if (glm::distance(my_transform->position, p1_transform->position) < 0.5f &&
				can_teleport && both_placed) {
				can_teleport = false;
				my_transform->position = p2_transform->position;
			}
			else if (glm::distance(my_transform->position, p2_transform->position) < 0.5f &&
				can_teleport && both_placed) {
				can_teleport = false;
				my_transform->position = p1_transform->position;
			}
			else if (glm::distance(my_transform->position, p1_transform->position) > 0.5f &&
				glm::distance(my_transform->position, p2_transform->position) > 0.5f) {
				can_teleport = true;
			}
		}
		// movement disabled
		else {
			pre_move = false;
		}

		//attack command
		hit_id = 0;
		if (hitTimer <= 0.0f && blockTimer <= 0.0f && stunTimer <= 0.0f) {
			//initialize attack
			if (attack.pressed) {
				hitTimer = hitCD;
				animation_machines[my_id - 1].set_state(HIT_1);
			}
		}
		else if (hitTimer > 0.0f){
			float updatedHitTimer = hitTimer - elapsed;
			if (hitTimer > hitCD - hitWindup && updatedHitTimer <= hitCD - hitWindup) {
				hit_id = collisionSystem->CheckOverLap(my_id, attackDegree, attackRadius);
				// if I hit something, play hit sound
				if (hit_id != 0) {
					Sound::play_3D(*hit_sample, FX_VOL, players_transform[my_id-1]->position, 1.0f);
					combat_timer = MAX_COMBAT_TIME;
					if (!is_in_combat) {
						is_in_combat = true;
						background_music->stop(1.0f);
						combat_music = Sound::loop(*grime_sample, COMBAT_VOL);
					}
				}
			}
			hitTimer -= elapsed;
		}

		//block command
		if (hitTimer <= 0.0f && blockTimer <= 0.0f && stunTimer <= 0.0f) {
			if (block.pressed) {
				blockTimer = blockCD;
				animation_machines[my_id - 1].set_state(BLOCK);
			}
		}
		else if (blockTimer > 0.0f) {
			blockTimer -= elapsed;
		}

		//stun
		if (stunTimer > 0.0f) {
			stunTimer -= elapsed;
		}
		
		//fix overlap with other players
		collisionSystem->FixOverLap(my_id);

		//update players and portals position
		players_transform[my_id-1]->position = my_transform->position;
		players_transform[my_id-1]->rotation = my_transform->rotation;
		portal1_transform[my_id-1]->position = p1_transform->position;
		portal1_transform[my_id-1]->rotation = p1_transform->rotation;
		portal2_transform[my_id-1]->position = p2_transform->position;
		portal2_transform[my_id-1]->rotation = p2_transform->rotation;
		
		//update camera
		cameraController->UpdateCamera();
	}

	// sending my info to server:
	{
		// convert info to msg
		if (my_id == 0) {
			Client_Player myself(
				my_transform->position, my_transform->rotation, p1_transform->position, p1_transform->rotation,
				p2_transform->position, p2_transform->rotation, hit_id,
				IDLE, 0
			);
			std::vector<unsigned char> client_message;
			myself.convert_to_message(client_message);
			// send msg
			Connection & c = client.connections.back();
			c.send('b');
			c.send_buffer.insert(c.send_buffer.end(), client_message.begin(), client_message.end());
		}
		else {
			Client_Player myself(
				my_transform->position, my_transform->rotation, p1_transform->position, p1_transform->rotation,
				p2_transform->position, p2_transform->rotation, hit_id,
				animation_machines[my_id - 1].current_state, animation_machines[my_id - 1].current_frame
			);
			std::vector<unsigned char> client_message;
			myself.convert_to_message(client_message);
			// send msg
			Connection & c = client.connections.back();
			c.send('b');
			c.send_buffer.insert(c.send_buffer.end(), client_message.begin(), client_message.end());
		}
		
	}

	//receive data:
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
			bool gotHit;
			AnimationState animState;
			unsigned int current_frame;
			client_player.read_from_message(content, id, gotHit, animState, current_frame);

			// damage logic
			if (id == my_id) {
				// someone hit me
				if (!hitLastTime && gotHit) {
					combat_timer = MAX_COMBAT_TIME;
					if (!is_in_combat) {
						is_in_combat = true;
						background_music->stop(1.0f);
						combat_music = Sound::loop(*grime_sample, COMBAT_VOL);
					}
					
					// parry successfully
					if (blockTimer >= blockCD - blockZone) {
						std::cout << "parry!!" << std::endl;
						Sound::play_3D(*block_sample, FX_VOL, players_transform[my_id-1]->position, 1.0f);
					}
					// no parry
					else {
						health -= damage;
						stunTimer = stunCD;
						hitTimer = 0.0f;
						blockTimer = 0.0f;
						std::cout << "I am damaged!!" << std::endl;
						Sound::play_3D(*damage_sample, FX_VOL, players_transform[my_id-1]->position, 1.0f);
					}
				}
				hitLastTime = gotHit;	
			}

			// --------- process info ---------- //
			// is this my info ? (server will put my own info at first)
			if(i == i_offset){
				// if unkonwn before (following code only runs once)
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
					portal1_transform[id-1]->draw = true;
					portal2_transform[id-1]->draw = true;
					//adjust collision transform
					collisionSystem->elements[my_id - 1]->parent = my_transform;
				}
			}
			// other players' info, update their models' transform & portals
			else{
				players_transform[id-1]->draw = true;
				portal1_transform[id-1]->draw = true;
				portal2_transform[id-1]->draw = true;
				// positions
				players_transform[id-1]->position = client_player.position;
				portal1_transform[id-1]->position = client_player.portal1_position;
				portal2_transform[id-1]->position = client_player.portal2_position;
				// rotations
				players_transform[id-1]->rotation = client_player.rotation;
				portal1_transform[id-1]->rotation = client_player.portal1_rotation;
				portal2_transform[id-1]->rotation = client_player.portal2_rotation;

				if (animState != animation_machines[id-1].current_state) {
					animation_machines[id-1].set_state(animState);
				}
			}

			// move to next player's info
			i += Server_Player::Server_Player_mes_size;
		}

		// game logic 

	}

	// std::cerr << "Finished update()\n";

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	// std::cerr << "draw()\n";
	if(my_id ==0)
		return;
	// std::cerr << "actual draw()\n";
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

	scene.draw(*my_camera, my_id);

	// text
	glDisable(GL_DEPTH_TEST);
	hintFont->draw("Your Are Player " + std::to_string(my_id), 20.0f, 550.0f, glm::vec2(0.2,0.25), glm::vec3(0.2, 0.8f, 0.2f));
	if(ping)
		heart->Draw(glm::vec2(190.0f, 530.0f), glm::vec2(30.0f, 30.0f), 0.0f, glm::vec3(1.0f, .8f, .8f));
	else
		sword->Draw(glm::vec2(190.0f, 530.0f), glm::vec2(30.0f, 30.0f), 0.0f, glm::vec3(0.8f, .8f, 1.0f));

	GL_ERRORS();
	// std::cerr << "Finished draw()\n";
}
