#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <deque>
#include <vector>
#include <fstream>

struct BoneWeight {
	float weights[4];

	BoneWeight() {
		weights[0] = 0.0f;
		weights[1] = 0.0f;
		weights[2] = 0.0f;
		weights[3] = 0.0f;
	}

	void insert(float weight) {
		for (int i = 0; i < 4; i++) {
			if (weights[i] == 0.0f) {
				weights[i] = weight;
				break;
			}
		}
	}
};


struct BoneID {
	int ids[4];

	BoneID() {
		ids[0] = -1;
		ids[1] = -1;
		ids[2] = -1;
		ids[3] = -1;
	}

	void insert(int id) {
		for (int i = 0; i < 4; i++) {
			if (ids[i] == -1) {
				ids[i] = id;
				break;
			}
		}
	}
};

struct Bone {
    int node_id;
    glm::mat4 inverse_binding;
    Bone(int n, const glm::mat4& i) : node_id(n), inverse_binding(i) {}
	Bone() = default;
};

struct Node {
    bool has_animation = false;
    int animation_id = 0;
    int parent_id;
    glm::mat4 transform;
    glm::mat4 overall_transform;
    Node(unsigned int p, const glm::mat4& t) : parent_id(p), transform(t) {}
	Node() = default;
};

// ik it's wasteful. Whatever.
constexpr int NUM_MAX_FRAMES = 180;
struct Animation {
    int node_id;
    int num_frames;
    glm::mat4 keys[NUM_MAX_FRAMES];
};