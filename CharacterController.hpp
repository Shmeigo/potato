#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <math.h>

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

};