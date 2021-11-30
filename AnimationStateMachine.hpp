#pragma once
// generated from animation_state_machine.py

#include <map>
#include <vector>




enum AnimationState {
	IDLE,
	RUN,
	HIT_1,
	BLOCK
};

struct AnimationStateMachine {
	AnimationState current_state = IDLE;
	std::map<AnimationState, std::pair<unsigned, unsigned>> times;
	std::map<AnimationState, bool> is_oneshot;
	unsigned current_frame;
	bool state_has_changed = false;

	AnimationStateMachine()  {
		times[IDLE] = {0, 29};
		times[RUN] = {40, 59};
		times[HIT_1] = {70, 99};
		times[BLOCK] = {100, 129};
		is_oneshot[IDLE] = false;
		is_oneshot[RUN] = false;
		is_oneshot[HIT_1] = true;
		is_oneshot[BLOCK] = true;
		current_frame = times[IDLE].first;
	}

	void update() {
		const auto& time = times[current_state];
		if (state_has_changed) {
			current_frame = time.first;
			state_has_changed = false;
		}
		else {
			current_frame++;
			if (current_frame > time.second) {
				if (is_oneshot[current_state]) {
					current_state = IDLE;
					current_frame = times[IDLE].first;
				}
				else {
					current_frame = time.first;
				}
			}
		}
	}

	void set_state(AnimationState state) {
		current_state = state;
		state_has_changed = true;
	}
};

