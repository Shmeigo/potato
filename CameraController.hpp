#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <iostream>

class CameraController {
public:
	CameraController(
		Scene::Camera* camera_, Scene::Transform* target_, glm::vec3 offset_,
		float bottom_radius_, float middle_radius_, float top_radius_,
		float bottom_height_, float middle_height_, float top_height_
	) : camera(camera_), target(target_), offset(offset_),
		bottom_radius(bottom_radius_), middle_radius(middle_radius_), top_radius(top_radius_),
		bottom_height(bottom_height_), middle_height(middle_height_), top_height(top_height_),
		horizontal_angle(0.0f), vertical_angle(0.0f)
	{
		vertical_angle_max = glm::degrees(glm::atan(top_height - middle_height, top_radius));
		vertical_angle_min = -glm::degrees(glm::atan(middle_height - bottom_height, bottom_radius));
		AdjustCamera(glm::vec2(0.0f));
	}

	void AdjustCamera(glm::vec2 camera_movement_);

	void UpdateCamera();

private:
	Scene::Camera* camera = nullptr;
	Scene::Transform* target = nullptr;

	[[maybe_unused]]
	glm::vec3 offset = glm::vec3(0.0f);

	glm::vec3 camera_position;
	glm::quat camera_rotation;

	float horizontal_speed = 45.0f;
	float vertical_speed = 30.0f;

	float vertical_angle_max;
	float vertical_angle_min;

	float bottom_radius;
	float middle_radius;
	float top_radius;
	float bottom_height;
	float middle_height;
	float top_height;
	
	float horizontal_angle;
	float vertical_angle;
	
};