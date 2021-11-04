#include "CameraController.hpp"

void CameraController::AdjustCamera(glm::vec2 camera_movement_) {
	//update horizontal and vertical angle
	horizontal_angle += horizontal_speed * camera_movement_.x;
	vertical_angle -= vertical_speed * camera_movement_.y;
	vertical_angle = std::min(vertical_angle, vertical_angle_max);
	vertical_angle = std::max(vertical_angle, vertical_angle_min);

	//calculate camera local position and rotation
	//camera orbit radius
	float radius;
	float ratio;
	if (vertical_angle > 0.0f) {
		ratio = vertical_angle / vertical_angle_max;
		radius = ratio * top_radius + (1.0f - ratio) * middle_radius;
	}
	else {
		ratio = vertical_angle / vertical_angle_min;
		radius = ratio * bottom_radius + (1.0f - ratio) * middle_radius;
	}
	//camera local position
	camera_position.z = radius * glm::sin(glm::radians(vertical_angle)) + middle_height;
	float radius_xy = radius * glm::cos(glm::radians(vertical_angle));
	camera_position.x = radius_xy * glm::sin(glm::radians(horizontal_angle));
	camera_position.y = radius_xy * glm::cos(glm::radians(horizontal_angle));

	//camera orientation
	float pitch = glm::radians(-vertical_angle + 90.0f);
	float yaw = glm::radians(-horizontal_angle + 180.0f);
	camera_rotation = glm::angleAxis(yaw, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
}

void CameraController::UpdateCamera() {
	camera->transform->position = target->position + camera_position;
	camera->transform->rotation = camera_rotation;
}