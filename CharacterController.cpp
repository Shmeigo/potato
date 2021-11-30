#include "CharacterController.hpp"

CharacterController::CharacterController(Scene::Transform* character_, Scene::Camera* camera_) : 
	character(character_), camera(camera_), 
	velocity(glm::vec3(0.0f)), current_angle(0.0f), target_angle(0.0f)
{ 
	// load heightmap and collisionmap
	std::ifstream collision_in(data_path("map/collision.txt"));
	int count = 0;
	std::string irrelevant;
	while(collision_in >> irrelevant) {
		assert(count < 2048 * 2048);
		collision_map[count] = (float)::atof(irrelevant.c_str());
		count++;
	}
	assert(count == 2048*2048);

	character->position = glm::vec3(0, 0, 0);
}

void CharacterController::UpdateCharacter(glm::vec2 movement, float elapsed) {
	if (movement != glm::vec2(0.0f)) {
		//normalize movement
		glm::vec2 move = glm::normalize(movement);

		//transfer movement to camera coordinate
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 forward = -frame[2];
		forward = glm::normalize(glm::vec3(forward.x, forward.y, 0.0f));
		glm::vec3 direction = move.x * right + move.y * forward;

		//clear z component
		direction.z = 0.0f;

		//update velocity
		velocity += direction * acceleration * elapsed;
		if (glm::length(velocity) > walking_speed_max) {
			velocity = walking_speed_max * glm::normalize(velocity);
		}
		
		//update target angle
		target_angle = glm::degrees(glm::atan(velocity.y, velocity.x)) - 90.0f;
	}
	//decrease velocity using damping
	else {
		if (velocity != glm::vec3(0.0f)) {
			glm::vec3 DampedVelocity = velocity - glm::normalize(velocity) * damp * elapsed;

			// make sure damping does not change the direction
			if ((velocity.x > 0 && DampedVelocity.x < 0) || (velocity.x < 0 && DampedVelocity.x > 0))
				DampedVelocity.x = 0.0f;
			if ((velocity.y > 0 && DampedVelocity.y < 0) || (velocity.y < 0 && DampedVelocity.y > 0))
				DampedVelocity.y = 0.0f;

			velocity = DampedVelocity;
		}
	}

	//update current angle
	target_angle = std::fmod(target_angle + 360.0f, 360.0f);
	if (target_angle > current_angle) {
		if (target_angle - current_angle < 180.0f) {
			current_angle = std::min(current_angle + angle_speed * elapsed, target_angle);
		}
		else {
			current_angle = std::max(current_angle - angle_speed * elapsed, target_angle - 360.0f);
		}
	}
	else {
		if (current_angle - target_angle < 180.0f) {
			current_angle = std::max(current_angle - angle_speed * elapsed, target_angle);
		}
		else {
			current_angle = std::min(current_angle + angle_speed * elapsed, target_angle + 360.0f);
		}
	}
	current_angle = std::fmod(current_angle + 360.0f, 360.0f);
	

	//update position
	glm::vec3 old_position = character->position;
	character->position += velocity * elapsed;

	// map -20 to 20 to 0 to 2048
	// why is it -20 to 20? Not quite sure, it may be something to do with the scaling in blender
	// but -20 to 20 seems to work fine
	unsigned coord_x = unsigned(((character->position.x + 20.0f)/40.0f) * 2048.0f);
	unsigned coord_y = unsigned(((character->position.y + 20.0f)/40.0f) * 2048.0f);
	float coll = 1.0f;
	if(coord_x * 2048 + (2048-coord_y) >=0 && coord_x * 2048 + (2048-coord_y) < collision_map.size())
		coll = collision_map[coord_x * 2048 + (2048-coord_y)];
	
	//std::cerr << character->position.x << " " << character->position.y << " " << coll << std::endl;
	
	if (coll < 0.95f) {
		character->position = old_position;
	}
	//update player orientation
	character->rotation = glm::angleAxis(glm::radians(current_angle), glm::vec3(0.0f, 0.0f, 1.0f));

	// start char at different position to not get stuck in map
/*	if (!done) {
		character->position = glm::vec3(1, 0, 0);
		done = true;
	}*/
} 