#include "CollisionSystem.hpp"

void CollisionSystem::Collidable::FixOverLap() {
	for (int i = 0; i < system->elements.size(); i++) {
		Collidable* other = system->elements[i];
		if (this == system->elements[i] || !other->parent->draw)
			continue;

		glm::vec3 diff_vec = parent->position - other->parent->position;
		float distance = length(diff_vec) - radius - other->radius;

		if (distance < 0) {
			//reset ball position
			glm::vec3 diff_pos = (-distance) * glm::normalize(diff_vec);
			parent->position += diff_pos;
		}
		
	}
}


int CollisionSystem::CheckOverLap(int CollidableID, float attackDegree, float attackRadius) {
	float minDistance = attackRadius;
	int target = 0;
	Collidable* current = elements[CollidableID - 1];
	glm::mat4x3 frame = current->parent->make_local_to_parent();
	glm::vec3 forward = glm::normalize(frame[1]);

	//std::cout << forward.x << " " << forward.y << " " << forward.z << std::endl;
	
	for (int i = 0; i < elements.size(); i++) {
		//get the other collider
		Collidable* other = elements[i];

		//skip self and inactive colliders
		if (current == other || !other->parent->draw)
			continue;

		//calculate distance
		glm::vec3 diff_vec = other->parent->position - current->parent->position;
		float distance = length(diff_vec);

		//skip out of range colliders
		if (distance > attackRadius)
			continue;

		//calculate angle
		diff_vec = glm::normalize(diff_vec);
		float angle = glm::degrees(glm::acos(glm::dot(diff_vec, forward)));

		//skip out of degree colliders
		if (angle > attackDegree * 0.5f)
			continue;

		if (distance < minDistance) {
			minDistance = distance;
			target = i + 1;
		}

		//std::cout << diff_vec.x << " " << diff_vec.y << " " << diff_vec.z << std::endl;
		//std::cout << "" << angle << std::endl;		

	}

	return target;
}