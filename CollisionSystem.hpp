#pragma once

#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

class CollisionSystem {
public:
	struct Collidable {
		Collidable(CollisionSystem * system_, Scene::Transform* parent_, float radius_) : system(system_), parent(parent_), radius(radius_){}

		CollisionSystem* system;
		Scene::Transform* parent;
		float radius;
	};

	CollisionSystem() {}
	void AddElement(Collidable* element) { elements.push_back(element); }

private:
	std::vector<Collidable*> elements;  

};