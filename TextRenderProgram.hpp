#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"
#include <glm/gtc/type_ptr.hpp>

//Shader program that draws transformed, vertices tinted with vertex colors:
struct TextRenderProgram {
	TextRenderProgram();
	~TextRenderProgram();

	GLuint program = 0;
};

extern Load<TextRenderProgram> text_render_program;