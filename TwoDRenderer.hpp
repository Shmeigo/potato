#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

struct TwoDRendererProgram {
	TwoDRendererProgram();
	~TwoDRendererProgram();

	GLuint program = 0;
};

extern Load<TwoDRendererProgram> two_d_render_program;

class TwoDRenderer
{
    GLuint VAO, VBO;
    GLuint texture;
public:
    TwoDRenderer(std::string imageFile);
    ~TwoDRenderer();
    void Draw( glm::vec2 position, glm::vec2 size, float rotate, glm::vec3 color);

};