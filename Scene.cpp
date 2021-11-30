#include "Scene.hpp"

#include "gl_errors.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>

//-------------------------

glm::mat4x3 Scene::Transform::make_local_to_parent() const {
	//compute:
	//   translate   *   rotate    *   scale
	// [ 1 0 0 p.x ]   [       0 ]   [ s.x 0 0 0 ]
	// [ 0 1 0 p.y ] * [ rot   0 ] * [ 0 s.y 0 0 ]
	// [ 0 0 1 p.z ]   [       0 ]   [ 0 0 s.z 0 ]
	//                 [ 0 0 0 1 ]   [ 0 0   0 1 ]

	glm::mat3 rot = glm::mat3_cast(rotation);
	return glm::mat4x3(
		rot[0] * scale.x, //scaling the columns here means that scale happens before rotation
		rot[1] * scale.y,
		rot[2] * scale.z,
		position
	);
}

glm::mat4x3 Scene::Transform::make_parent_to_local() const {
	//compute:
	//   1/scale       *    rot^-1   *  translate^-1
	// [ 1/s.x 0 0 0 ]   [       0 ]   [ 0 0 0 -p.x ]
	// [ 0 1/s.y 0 0 ] * [rot^-1 0 ] * [ 0 0 0 -p.y ]
	// [ 0 0 1/s.z 0 ]   [       0 ]   [ 0 0 0 -p.z ]
	//                   [ 0 0 0 1 ]   [ 0 0 0  1   ]

	glm::vec3 inv_scale;
	//taking some care so that we don't end up with NaN's , just a degenerate matrix, if scale is zero:
	inv_scale.x = (scale.x == 0.0f ? 0.0f : 1.0f / scale.x);
	inv_scale.y = (scale.y == 0.0f ? 0.0f : 1.0f / scale.y);
	inv_scale.z = (scale.z == 0.0f ? 0.0f : 1.0f / scale.z);

	//compute inverse of rotation:
	glm::mat3 inv_rot = glm::mat3_cast(glm::inverse(rotation));

	//scale the rows of rot:
	inv_rot[0] *= inv_scale;
	inv_rot[1] *= inv_scale;
	inv_rot[2] *= inv_scale;

	return glm::mat4x3(
		inv_rot[0],
		inv_rot[1],
		inv_rot[2],
		inv_rot * -position
	);
}

glm::mat4x3 Scene::Transform::make_local_to_world() const {
	if (!parent) {
		return make_local_to_parent();
	} else {
		return parent->make_local_to_world() * glm::mat4(make_local_to_parent()); //note: glm::mat4(glm::mat4x3) pads with a (0,0,0,1) row
	}
}
glm::mat4x3 Scene::Transform::make_world_to_local() const {
	if (!parent) {
		return make_parent_to_local();
	} else {
		return make_parent_to_local() * glm::mat4(parent->make_world_to_local()); //note: glm::mat4(glm::mat4x3) pads with a (0,0,0,1) row
	}
}

//-------------------------

glm::mat4 Scene::Camera::make_projection() const {
	return glm::infinitePerspective( fovy, aspect, near );
}

//-------------------------


void Scene::draw(Camera const &camera, uint8_t my_id) const {
	assert(camera.transform);
	glm::mat4 world_to_clip = camera.make_projection() * glm::mat4(camera.transform->make_world_to_local());
	glm::mat4x3 world_to_light = glm::mat4x3(1.0f);
	draw(my_id, world_to_clip, world_to_light);
}

void Scene::draw(uint8_t my_id, glm::mat4 const &world_to_clip, glm::mat4x3 const &world_to_light) const {

	// render to color and depth textures
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	GL_ERRORS();
	glEnable(GL_DEPTH_TEST);
	GL_ERRORS();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_ERRORS();

	//Iterate through all drawables, sending each one to OpenGL:
	for (auto const &drawable : drawables) {
		//Reference to drawable's pipeline for convenience:
		Scene::Drawable::Pipeline const &pipeline = drawable.pipeline;

		//skip any drawables without a shader program set:
		if (pipeline.program == 0) continue;
		//skip any drawables that don't reference any vertex array:
		if (pipeline.vao == 0) continue;
		//skip any drawables that don't contain any vertices:
		if (pipeline.count == 0) continue;

		// skip if specified not to draw
		if(!drawable.transform->draw) continue;

		//Set shader program:
		glUseProgram(pipeline.program);

		//Set attribute sources:
		glBindVertexArray(pipeline.vao);

		//Configure program uniforms:

		//the object-to-world matrix is used in all three of these uniforms:
		assert(drawable.transform); //drawables *must* have a transform
		glm::mat4x3 object_to_world = drawable.transform->make_local_to_world();

		//OBJECT_TO_CLIP takes vertices from object space to clip space:
		if (pipeline.OBJECT_TO_CLIP_mat4 != -1U) {
			glm::mat4 object_to_clip = world_to_clip * glm::mat4(object_to_world);
			glUniformMatrix4fv(pipeline.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(object_to_clip));
		}

		//the object-to-light matrix is used in the next two uniforms:
		glm::mat4x3 object_to_light = world_to_light * glm::mat4(object_to_world);

		//OBJECT_TO_CLIP takes vertices from object space to light space:
		if (pipeline.OBJECT_TO_LIGHT_mat4x3 != -1U) {
			glUniformMatrix4x3fv(pipeline.OBJECT_TO_LIGHT_mat4x3, 1, GL_FALSE, glm::value_ptr(object_to_light));
		}

		//NORMAL_TO_CLIP takes normals from object space to light space:
		if (pipeline.NORMAL_TO_LIGHT_mat3 != -1U) {
			glm::mat3 normal_to_light = glm::inverse(glm::transpose(glm::mat3(object_to_light)));
			glUniformMatrix3fv(pipeline.NORMAL_TO_LIGHT_mat3, 1, GL_FALSE, glm::value_ptr(normal_to_light));
		}

		//set any requested custom uniforms:
		if (pipeline.set_uniforms) pipeline.set_uniforms();

		//set up textures:
		for (uint32_t i = 0; i < Drawable::Pipeline::TextureCount; ++i) {
			if (pipeline.textures[i].texture != 0) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(pipeline.textures[i].target, pipeline.textures[i].texture);
			}
		}

		//draw the object:
		glDrawArrays(pipeline.type, pipeline.start, pipeline.count);

		//un-bind textures:
		for (uint32_t i = 0; i < Drawable::Pipeline::TextureCount; ++i) {
			if (pipeline.textures[i].texture != 0) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(pipeline.textures[i].target, 0);
			}
		}
		glActiveTexture(GL_TEXTURE0);

	}

	glUseProgram(0);
	glBindVertexArray(0);

	// iterate through all skeletals, sending each one to OpenGL
	for (const auto& skeletal : skeletals) {

		// skip if specified not to draw
		if(!skeletal.transform->draw){
			continue;
		}

		glUseProgram(skeletal.program);
		GL_ERRORS();
		glm::mat4x3 object_to_world = skeletal.transform->make_local_to_world();
		glm::mat4 object_to_clip = world_to_clip * glm::mat4(object_to_world);

		unsigned int mvp_loc = glGetUniformLocation(skeletal.program, "MVP");
		glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, (const float*)&object_to_clip);

		GL_ERRORS();
		for (const auto& mesh : skeletal.meshes) {
			unsigned int t_loc = glGetUniformLocation(skeletal.program, "BoneTransforms");
			glUniformMatrix4fv(t_loc, (GLsizei)mesh.bone_transforms.size(), GL_FALSE, (const float*)mesh.bone_transforms.data());
			GL_ERRORS();
			glBindVertexArray(mesh.vao);
			GL_ERRORS();
			glDrawElements(GL_TRIANGLES, mesh.elements, GL_UNSIGNED_INT, 0);
			GL_ERRORS();
		}
	}

	// now bind the default framebuffer and render the quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GL_ERRORS();
	glUseProgram(quad_program);
	GL_ERRORS();
	glBindVertexArray(quad_vao);
	GL_ERRORS();
	unsigned int texloc = glGetUniformLocation(quad_program, "ScreenTexture");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glUniform1i(texloc, 1);
	GL_ERRORS();
	unsigned int depthloc = glGetUniformLocation(quad_program, "ScreenDepth");
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glUniform1i(depthloc, 2);
	GL_ERRORS();
	
	// draw the quad
	glDrawArrays(GL_TRIANGLES, 0, 18);
	GL_ERRORS();
	glBindVertexArray(0);
	glUseProgram(0);
}


void Scene::load(std::string const &filename,
	std::function< void(Scene &, Transform *, std::string const &) > const &on_drawable) {

	std::ifstream file(filename, std::ios::binary);

	std::vector< char > names;
	read_chunk(file, "str0", &names);

	struct HierarchyEntry {
		uint32_t parent;
		uint32_t name_begin;
		uint32_t name_end;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
	};
	static_assert(sizeof(HierarchyEntry) == 4 + 4 + 4 + 4*3 + 4*4 + 4*3, "HierarchyEntry is packed.");
	std::vector< HierarchyEntry > hierarchy;
	read_chunk(file, "xfh0", &hierarchy);

	struct MeshEntry {
		uint32_t transform;
		uint32_t name_begin;
		uint32_t name_end;
	};
	static_assert(sizeof(MeshEntry) == 4 + 4 + 4, "MeshEntry is packed.");
	std::vector< MeshEntry > meshes;
	read_chunk(file, "msh0", &meshes);

	struct CameraEntry {
		uint32_t transform;
		char type[4]; //"pers" or "orth"
		float data; //fov in degrees for 'pers', scale for 'orth'
		float clip_near, clip_far;
	};
	static_assert(sizeof(CameraEntry) == 4 + 4 + 4 + 4 + 4, "CameraEntry is packed.");
	std::vector< CameraEntry > cameras;
	read_chunk(file, "cam0", &cameras);

	struct LightEntry {
		uint32_t transform;
		char type;
		glm::u8vec3 color;
		float energy;
		float distance;
		float fov;
	};
	static_assert(sizeof(LightEntry) == 4 + 1 + 3 + 4 + 4 + 4, "LightEntry is packed.");
	std::vector< LightEntry > lights;
	read_chunk(file, "lmp0", &lights);


	//--------------------------------
	//Now that file is loaded, create transforms for hierarchy entries:

	std::vector< Transform * > hierarchy_transforms;
	hierarchy_transforms.reserve(hierarchy.size());

	for (auto const &h : hierarchy) {
		transforms.emplace_back();
		Transform *t = &transforms.back();
		if (h.parent != -1U) {
			if (h.parent >= hierarchy_transforms.size()) {
				throw std::runtime_error("scene file '" + filename + "' did not contain transforms in topological-sort order.");
			}
			t->parent = hierarchy_transforms[h.parent];
		}

		if (h.name_begin <= h.name_end && h.name_end <= names.size()) {
			t->name = std::string(names.begin() + h.name_begin, names.begin() + h.name_end);
		} else {
				throw std::runtime_error("scene file '" + filename + "' contains hierarchy entry with invalid name indices");
		}

		t->position = h.position;
		t->rotation = h.rotation;
		t->scale = h.scale;

		hierarchy_transforms.emplace_back(t);
	}
	assert(hierarchy_transforms.size() == hierarchy.size());

	for (auto const &m : meshes) {
		if (m.transform >= hierarchy_transforms.size()) {
			throw std::runtime_error("scene file '" + filename + "' contains mesh entry with invalid transform index (" + std::to_string(m.transform) + ")");
		}
		if (!(m.name_begin <= m.name_end && m.name_end <= names.size())) {
			throw std::runtime_error("scene file '" + filename + "' contains mesh entry with invalid name indices");
		}
		std::string name = std::string(names.begin() + m.name_begin, names.begin() + m.name_end);

		if (on_drawable) {
			on_drawable(*this, hierarchy_transforms[m.transform], name);
		}

	}

	for (auto const &c : cameras) {
		if (c.transform >= hierarchy_transforms.size()) {
			throw std::runtime_error("scene file '" + filename + "' contains camera entry with invalid transform index (" + std::to_string(c.transform) + ")");
		}
		if (std::string(c.type, 4) != "pers") {
			std::cout << "Ignoring non-perspective camera (" + std::string(c.type, 4) + ") stored in file." << std::endl;
			continue;
		}
		this->cameras.emplace_back(hierarchy_transforms[c.transform]);
		Camera *camera = &this->cameras.back();
		camera->fovy = c.data / 180.0f * 3.1415926f; //FOV is stored in degrees; convert to radians.
		camera->near = c.clip_near;
		//N.b. far plane is ignored because cameras use infinite perspective matrices.
	}

	for (auto const &l : lights) {
		if (l.transform >= hierarchy_transforms.size()) {
			throw std::runtime_error("scene file '" + filename + "' contains lamp entry with invalid transform index (" + std::to_string(l.transform) + ")");
		}
		if (l.type == 'p') {
			//good
		} else if (l.type == 'h') {
			//fine
		} else if (l.type == 's') {
			//okay
		} else if (l.type == 'd') {
			//sure
		} else {
			std::cout << "Ignoring unrecognized lamp type (" + std::string(&l.type, 1) + ") stored in file." << std::endl;
			continue;
		}
		this->lights.emplace_back(hierarchy_transforms[l.transform]);
		Light *light = &this->lights.back();
		light->type = static_cast<Light::Type>(l.type);
		light->energy = glm::vec3(l.color) / 255.0f * l.energy;
		light->spot_fov = l.fov / 180.0f * 3.1415926f; //FOV is stored in degrees; convert to radians.
	}

	//load any extra that a subclass wants:
	load_extra(file, names, hierarchy_transforms);

	if (file.peek() != EOF) {
		std::cerr << "WARNING: trailing data in scene file '" << filename << "'" << std::endl;
	}



}

//-------------------------

Scene::Scene(std::string const &filename, std::function< void(Scene &, Transform *, std::string const &) > const &on_drawable) {
	load(filename, on_drawable);

	// load the quad program
	[[maybe_unused]]
	const char* vertex_shader_postprocess = "#version 330 core\n"
"layout (location = 0) in vec4 Position;\n"
"void main() {\n"
"	gl_Position = Position;\n"
"}\n";

	[[maybe_unused]]
	const char* fragment_shader_postprocess = 
"#version 330 core\n"
"precision mediump float;\n"
"uniform sampler2D ScreenTexture;\n"
"uniform sampler2D ScreenDepth;\n"
"vec4 getTextureColor(float x, float y) {\n"
"    return texture(ScreenTexture, vec2(x/1920.0f, y/1080.0f));\n"
"}\n"
"vec4 getTextureDepth(float x, float y) {\n"
"    return texture(ScreenDepth, vec2(x/1920.0f, y/1080.0f));\n"
"}\n"
"out vec4 FragColor;\n"
"void main() {\n"
	"float c = getTextureDepth(gl_FragCoord.x-3.0f, gl_FragCoord.y).x;\n"
	"float c1 = getTextureDepth(gl_FragCoord.x+3.0f, gl_FragCoord.y).x;\n"
	"float u = getTextureDepth(gl_FragCoord.x, gl_FragCoord.y-2.0f).x;\n"
	"float u1 = getTextureDepth(gl_FragCoord.x, gl_FragCoord.y+2.0f).x;\n"
	"float dx = abs(c - c1);\n"
	"float dy = abs(u - u1);\n"
	"if (dy > dx && dy > 0.0005f) {\n"
	"	FragColor = vec4(1.0f, 0, 0.48f, 1);\n"
	"}\n"
	"else if (dx > dy && dx > 0.0005f) {\n"
	"	FragColor = vec4(0.466f, 0.85f, 0.44f, 1);\n"
	"}\n"
	"else {\n"
	"	FragColor = getTextureColor(gl_FragCoord.x, gl_FragCoord.y);\n"
	"}\n"
"}\n";

	int status;

	int vshader_post = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader_post, 1, &vertex_shader_postprocess, NULL);
	glCompileShader(vshader_post);
	glGetShaderiv(vshader_post, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetShaderInfoLog(vshader_post, 1024, &l, log);
		std::cerr << log << std::endl;
	}

	int fshader_post = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader_post, 1, &fragment_shader_postprocess, NULL);
	glCompileShader(fshader_post);

	glGetShaderiv(fshader_post, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetShaderInfoLog(fshader_post, 1024, &l, log);
		std::cerr << log << std::endl;
	}

	quad_program = glCreateProgram();
	glAttachShader(quad_program, vshader_post);
	glAttachShader(quad_program, fshader_post);
	glLinkProgram(quad_program);

	glGetProgramiv(quad_program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetProgramInfoLog(quad_program, 1024, &l, log);
		std::cerr << log << std::endl;
	}

	float quad[18] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};

	glGenVertexArrays(1, &quad_vao);
	unsigned int quad_vbo;
	glGenBuffers(1, &quad_vbo);
	
	glBindVertexArray(quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, 18 * 4, quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	GL_ERRORS();

	// initialize FBO
	// allocate a new framebuffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// allocate a new texture to hold the render output
	glGenTextures(1, &color_tex);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// I've left this here for reference, but we can't use a renderbuffer any more
	// allocate a renderbuffer to store depth output for depth testing
	// glGenRenderbuffers(1, &ren);
	// glBindRenderbuffer(GL_RENDERBUFFER, ren);
	// glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1920, 1080);
	// glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ren);
	glGenTextures(1, &depth_tex);
	glBindTexture(GL_TEXTURE_2D, depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1920, 1080, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	// attach texture to framebuffer's color buffer at position 0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);
	
	unsigned int drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
	glDrawBuffers(2, drawBuffers);

	// was the framebuffer constructed properly?
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Something went wrong with creating the framebuffer\n";
	}
	else {
		std::cerr << "Framebuffer created successfully!\n";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

Scene::Scene(Scene const &other) {
	set(other);
}

Scene &Scene::operator=(Scene const &other) {
	set(other);
	return *this;
}

void Scene::set(Scene const &other, std::unordered_map< Transform const *, Transform * > *transform_map_) {

	std::unordered_map< Transform const *, Transform * > t2t_temp;
	std::unordered_map< Transform const *, Transform * > &transform_to_transform = *(transform_map_ ? transform_map_ : &t2t_temp);

	transform_to_transform.clear();

	//null transform maps to itself:
	transform_to_transform.insert(std::make_pair(nullptr, nullptr));

	//Copy transforms and store mapping:
	transforms.clear();
	for (auto const &t : other.transforms) {
		transforms.emplace_back();
		transforms.back().name = t.name;
		transforms.back().position = t.position;
		transforms.back().rotation = t.rotation;
		transforms.back().scale = t.scale;
		transforms.back().parent = t.parent; //will update later

		//store mapping between transforms old and new:
		auto ret = transform_to_transform.insert(std::make_pair(&t, &transforms.back()));
		assert(ret.second);
	}

	//update transform parents:
	for (auto &t : transforms) {
		t.parent = transform_to_transform.at(t.parent);
	}

	//copy other's drawables, updating transform pointers:
	drawables = other.drawables;
	for (auto &d : drawables) {
		d.transform = transform_to_transform.at(d.transform);
	}

	//copy other's cameras, updating transform pointers:
	cameras = other.cameras;
	for (auto &c : cameras) {
		c.transform = transform_to_transform.at(c.transform);
	}

	//copy other's lights, updating transform pointers:
	lights = other.lights;
	for (auto &l : lights) {
		l.transform = transform_to_transform.at(l.transform);
	}

	// copy GL state
	fbo = other.fbo;
	quad_program = other.quad_program;
	quad_vao = other.quad_vao;
	color_tex = other.color_tex;
	depth_tex = other.depth_tex;
}


// --- Skeletal ---
Scene::Skeletal::Skeletal(Scene::Transform* t) : transform(t) {
	[[maybe_unused]]
	const char* vertex_shader_pos = "#version 330 core\n"
"layout (location = 0) in vec4 Position;\n"
"uniform mat4 MVP;\n"
"void main() {\n"
"	gl_Position = MVP * Position;\n"
"}\n";

	[[maybe_unused]]
	const char* fragment_shader_pos = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"	FragColor = vec4(1, 1, 1, 1);\n"
"}\n";

const char* vertex_shader = "#version 330 core\n"
"layout (location = 0) in vec4 Position;\n"
"layout (location = 1) in ivec4 BoneIDs;\n"
"layout (location = 2) in vec4 BoneWeights;\n"
"layout (location = 3) in vec3 pass_Normal;\n"
"out vec3 Normal;\n"
"uniform mat4[64] BoneTransforms;\n"
"uniform mat4 MVP;\n"
"void main() {\n"
"	vec4 transformed = vec4(0, 0, 0, 1);\n"
"	for (int i = 0; i < 4; i++) {\n"
"		int index = BoneIDs[i];\n"
"		if (index != -1) transformed = transformed + BoneWeights[i] * BoneTransforms[index] * Position;\n"
"	}\n"
	"Normal = pass_Normal;\n"
"	gl_Position = MVP * transformed;\n"
"}\n";

const char* fragment_shader = "#version 330 core\n"
"layout (location = 0) out vec4 FragColor;\n"
"in vec3 Normal;\n"
"void main() {\n"
"	float c = dot(Normal, normalize(vec3(1, 1, 0)));\n"
"	FragColor = vec4(0.09f, 0.15f, 0.454f, 1);\n"
"}\n";



	int status;

	// TODO: refactor these into a single function
	int vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, 1, &vertex_shader, NULL);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetShaderInfoLog(vshader, 1024, &l, log);
		std::cerr << log << std::endl;
	}

	int fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, 1, &fragment_shader, NULL);
	glCompileShader(fshader);

	glGetShaderiv(fshader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetShaderInfoLog(fshader, 1024, &l, log);
		std::cerr << log << std::endl;
	}

	program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char log[1024] = {0};
		int l;
		glGetProgramInfoLog(program, 1024, &l, log);
		std::cerr << log << std::endl;
	}


	

	// don't transfer stuff rn
	std::vector<int> num_meshes;
	std::ifstream num_in(data_path("skeletal/num.dat"), std::ios::binary);
	read_chunk(num_in, "nums", &num_meshes);
	// std::cerr << "Num meshes: " << num_meshes[0] << std::endl;
	num_in.close();
	
	std::ifstream node_in(data_path("skeletal/nodes.dat"), std::ios::binary);
	read_chunk(node_in, "node", &nodes);
	node_in.close();

	std::ifstream animation_in(data_path("skeletal/animations.dat"), std::ios::binary);
	read_chunk(animation_in, "anim", &animations);
	animation_in.close();

	// std::cerr << "Num nodes: " << nodes.size() << std::endl;
	// std::cerr << "Num anims: " << animations.size() << std::endl;

	// make our player character actually stand up
	nodes[0].transform = glm::rotate(nodes[0].transform, 90 * 3.14159f/180.f, glm::vec3(1, 0, 0));
	nodes[0].transform = glm::rotate(nodes[0].transform, 180 * 3.14159f/180.f, glm::vec3(0, 0, 1));

	for (int i = 0; i < num_meshes[0]; i++) {
		meshes.emplace_back(i);
	}

	
}

Scene::Skeletal::AnimatedMesh::AnimatedMesh(int i) {
	std::string prefix = std::string("skeletal/mesh") + std::to_string(i);
	std::ifstream vin(data_path(prefix+std::string("vertices.dat")), std::ios::binary);
	std::ifstream nin(data_path(prefix+std::string("normals.dat")), std::ios::binary);
	std::ifstream iin(data_path(prefix+std::string("indices.dat")), std::ios::binary);
	std::ifstream win(data_path(prefix+std::string("weights.dat")), std::ios::binary);
	std::ifstream din(data_path(prefix+std::string("ids.dat")), std::ios::binary);
	std::ifstream bin(data_path(prefix+std::string("bones.dat")), std::ios::binary);

	// Realized way too late that all of these can read from just one input stream
	// oh well, not changing this now, too close to final	
	read_chunk(vin, "vert", &vertices);
	read_chunk(nin, "norm", &normals);
	read_chunk(iin, "indi", &indices);
	read_chunk(win, "weig", &bone_weights);
	read_chunk(din, "idss", &bone_ids);
	read_chunk(bin, "bone", &bones);

	bone_transforms.clear();
	for (int i = 0; i < bones.size(); i++) {
		bone_transforms.emplace_back();
	}

	vin.close();
	nin.close();
	iin.close();
	win.close();
	din.close();
	bin.close();

	// std::cerr << vertices.size() << ", " << normals.size() << ", " << indices.size() << ", " << bone_weights.size() << ", " << bone_ids.size() << ", " << bones.size() << std::endl;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &norm_vbo);
	glGenBuffers(1, &weight_vbo);
	glGenBuffers(1, &id_vbo);
	glGenBuffers(1, &ebo);

	elements = (unsigned int)indices.size();

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, id_vbo);
	glBufferData(GL_ARRAY_BUFFER, bone_ids.size() * sizeof(BoneID), bone_ids.data(), GL_STATIC_DRAW);
	glVertexAttribIPointer(1, 4, GL_INT, 4 * sizeof(int), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, weight_vbo);
	glBufferData(GL_ARRAY_BUFFER, bone_weights.size() * sizeof(BoneWeight), bone_weights.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, norm_vbo);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	// for (const auto& bone : bones) {
	// 	// std::cerr << bone.node_id << ", ";
	// }
	// // std::cerr << std::endl;

}

void Scene::Skeletal::update_nodes(int frame) {
	// std::cerr << "update nodes starts\n";
	for (int i = 0; i < nodes.size(); i++) {
		assert(i > nodes[i].parent_id);
		if (i == 0) {
			nodes[i].overall_transform = nodes[i].transform;
		}
		else if (nodes[i].has_animation) {
			const auto& anim_transform = animations[nodes[i].animation_id].keys[frame];
			nodes[i].overall_transform = nodes[nodes[i].parent_id].overall_transform * anim_transform;
		}
		else {
			nodes[i].overall_transform = nodes[nodes[i].parent_id].overall_transform * nodes[i].transform;
		}
	}

	// std::cerr << "update nodes ends\n";
	for (auto& mesh : meshes) {
		mesh.update_bone_transforms(nodes);
	}
	// std::cerr << "update bones ends\n";
}

void Scene::Skeletal::AnimatedMesh::update_bone_transforms(const std::vector<Node>& ns) {
	// std::cerr << "update bones starts\n";
	
	for (int i = 0; i < bones.size(); i++) {
		// std::cerr << "updating bone " << i << "\n";
		bone_transforms[i] = ns[bones[i].node_id].overall_transform * bones[i].inverse_binding;
	}
	// std::cerr << "update bones ends\n";
}