#include "CharacterController.hpp"

CharacterController::CharacterController(Scene::Transform* character_, Scene::Camera* camera_) : 
	character(character_), camera(camera_), 
	velocity(glm::vec3(0.0f)), current_angle(0.0f), target_angle(0.0f)
{ 
	
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
	character->position += velocity * elapsed;

	//update player orientation
	character->rotation = glm::angleAxis(glm::radians(current_angle), glm::vec3(0.0f, 0.0f, 1.0f));
} 