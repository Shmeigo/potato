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