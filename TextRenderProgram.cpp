/*
This file is based on these resources:
https://github.com/15-466/15-466-f21-base4
https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c 
https://www.freetype.org/freetype2/docs/tutorial/step1.html 
https://learnopengl.com/In-Practice/Text-Rendering
*/

#include "TextRenderProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load<TextRenderProgram> text_render_program(LoadTagEarly);

TextRenderProgram::TextRenderProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;\n"
        "} \n" 
	,
		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 fragColor;\n"
		"uniform sampler2D text;\n"
		"uniform vec3 textColor;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "   fragColor = vec4(textColor, 1.0) * sampled;\n"
		"}\n"
	); 
}

TextRenderProgram::~TextRenderProgram() {
	glDeleteProgram(program);
	program = 0;
}
