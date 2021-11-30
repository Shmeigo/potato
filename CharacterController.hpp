#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <math.h>
#include <array>
#include <fstream>
#include "data_path.hpp"

// // to make it easier to print vec3s
// std::ostream& operator<<(std::ostream& out, const glm::vec3& vec) {
// 	out << "[vec3 " << vec.x << " " << vec.y << " " << vec.z << "]";
// 	return out;
// }

class CharacterController {
public:
	CharacterController(Scene::Transform* character_, Scene::Camera* camera_);
	void UpdateCharacter(glm::vec2 movement, float elapsed);
	

	Scene::Transform* character;
	Scene::Camera* camera;

private:
	glm::vec3 velocity;
	float acceleration = 10.0f;
	float damp = 20.0f;
	float walking_speed_max = 2.0f;

	float current_angle;
	float target_angle;
	float angle_speed = 720.0f;
	[[maybe_unused]]
	bool done = false;
	std::array<float, 2048*2048> collision_map;
};