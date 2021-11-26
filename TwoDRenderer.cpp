# include "TwoDRenderer.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load<TwoDRendererProgram> two_d_render_program(LoadTagEarly);

TwoDRendererProgram::TwoDRendererProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 model\n;"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;\n"
        "} \n" 
	,
		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"uniform sampler2D image;\n"
		"uniform vec3 spriteColor;\n"
		"void main() {\n"
		"	color = vec4(spriteColor, 1.0) * texture(image, TexCoords);\n"
		"}\n"
	); 
}

TwoDRendererProgram::~TwoDRendererProgram() {
	glDeleteProgram(program);
	program = 0;
}

// ----------------------------------------------------------------- //

TwoDRenderer::TwoDRenderer(std::string imageFile){

    {
        // set up vao, vbo 
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // vertices
        float vertices[] = { 
            // pos      // tex
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 
        
            0.0f, 1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // set attribute in location 0, which is the in-vertex for vertex shader. See the definition in (TextRenderProgram, line.13)
        glEnableVertexAttribArray(0);
        // tells vao how to read from buffer. (read 4 floats each time, which is one vertex)
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        // done
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);    
    }

    {
        // generate buffer
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set some texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // upload image
        int width, height, nrChannels;
        unsigned char *data = stbi_load(imageFile.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

}

TwoDRenderer::~TwoDRenderer(){
    glDeleteBuffers(1,&VBO);
    glDeleteVertexArrays(1,&VAO);
    glDeleteTextures(1,&texture);
}

void TwoDRenderer::Draw( glm::vec2 position, 
  glm::vec2 size, float rotate, glm::vec3 color)
{

    glUseProgram(two_d_render_program->program);

    // prepare transformations
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position, 0.0f));  
    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); 
    model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f)); 
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f)); 
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
  
    // pass in uniforms
    glUniform3f(glGetUniformLocation(two_d_render_program->program, "spriteColor"), color.x, color.y, color.z);
    glUniformMatrix4fv(glGetUniformLocation(two_d_render_program->program,"projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(two_d_render_program->program,"model"), 1, GL_FALSE, glm::value_ptr(model));

    // rendering buffers setup
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // draw quat
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // done
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}  