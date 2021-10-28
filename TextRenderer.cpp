/*
This file is based on these resources:
https://github.com/15-466/15-466-f21-base4
https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c 
https://www.freetype.org/freetype2/docs/tutorial/step1.html 
https://learnopengl.com/In-Practice/Text-Rendering
https://github.com/GenBrg/MarryPrincess/blob/master/DrawFont.cpp
*/


#include "TextRenderer.hpp"

#define FONT_SIZE 100

TextRenderer::TextRenderer(std::string fontfile)
{
    // init & sanity check
    FT_Error ft_error;
    if ((ft_error = FT_Init_FreeType (&ft_library))){
        std::cerr << "FT lib init fail\n";
        abort();
    }
    if ((ft_error = FT_New_Face (ft_library, fontfile.c_str(), 0, &ft_face))){
        std::cerr << "FT face init fail\n";
        abort();
    }

    // font size we exctract from the face
    // if you want to extract bitmap at runtime, you can use this as a scaling method
    // But i choose to preload glyphs so I will set this here, and use a scale factor later to change size
    if ((ft_error = FT_Set_Char_Size (ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0)))
        abort();
    
    // disable alignment since what we read from the face (font) is grey-scale
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // constrcut the glyph map for common ascii-key chars
    setupCharacterGlyphMap();

    // setup vao,vbo for quad
    bufferSetup();
}

void TextRenderer::draw(std::string text, float x, float y, glm::vec2 scale, glm::vec3 color){
    // check if text diff
    if(text.compare(prevText) != 0){
        prevText = text;
        changeTextContent();    // update shaping with harfbuzz
    }

    // enable for text drawing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    // set the text rendering shaders
    glUseProgram(text_render_program->program);

    // pass in uniforms
    glUniform3f(glGetUniformLocation(text_render_program->program, "textColor"), color.x, color.y, color.z);
    // for the projection matrix we use orthognal
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);
    glUniformMatrix4fv(glGetUniformLocation(text_render_program->program,"projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // rendering buffers setup
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // go thru all glyphs and render them
    uint16_t i=0;
    for (char c : text)
    {
        // first get the hb shaping infos (offset & advance)
        float x_offset = pos[i].x_offset / 64.0f;
        float y_offset = pos[i].y_offset / 64.0f;
        float x_advance = pos[i].x_advance / 64.0f;
        float y_advance = pos[i].y_advance / 64.0f;

        // take out the glyph using char
        Glyph ch = CharacterGlyph[c];
        // calculate actual position
        float xpos = x + (x_offset + ch.Bearing.x) * scale.x;
        float ypos = y + (y_offset - (ch.Size.y - ch.Bearing.y)) * scale.y;
        float w = ch.Size.x * scale.x;
        float h = ch.Size.y * scale.y;

        // update VBO for each character (6 vertices to draw a quad, which holds a glyph)
        // the info for each vector is (pos_x, pox_y, texture_coord_x, texture_coord_y)
        // check my vertex shader at TextRenderProgram.cpp to see how it is used
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }            
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // advance to next graph, using the harfbuzz shaping info
        x += x_advance * scale.x;
        y += y_advance * scale.y;
        i++;
    }

    // unbind
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// called when the displaying text is changed, to use harfbuzz to reshape
// (This function is mainly based on: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c)
void TextRenderer::changeTextContent(){

    // free previous resources
    if(hb_buffer)
        hb_buffer_destroy(hb_buffer);
    if(hb_font)
        hb_font_destroy(hb_font);

    // recreate hb resources
    hb_font = hb_ft_font_create (ft_face, NULL);
    /* Create hb-buffer and populate. */
    hb_buffer = hb_buffer_create ();

    // reshape
    hb_buffer_add_utf8 (hb_buffer, prevText.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);
    hb_shape (hb_font, hb_buffer, NULL, 0);

    /* Get glyph information and positions out of the buffer. */
    info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
}

// constrcut the glyph map for common ascii-key chars
void TextRenderer::setupCharacterGlyphMap(){
    // go over ascii key 32-126
    for (unsigned char c = 32; c < 127; c++){
        // load character glyph (which contains bitmap)
        if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER)){
            std::cout << "Fail to load Glyph for: " << c << std::endl;
            continue;
        }
        // generate buffer
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        // upload the bitmap to texure buffer
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            ft_face->glyph->bitmap.width,
            ft_face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ft_face->glyph->bitmap.buffer
        );
        // set some texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // store into the map
        Glyph glyph = {
            texture, 
            glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
            glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top),
        };
        CharacterGlyph.insert(std::pair<char, Glyph>(c, glyph));
    }
}

// as it is
void TextRenderer::destroyCharacterGlyphMap(){
    for (unsigned char c = 0; c < 128; c++)
    {
        glDeleteTextures(1, &CharacterGlyph[c].textureID);
    }
}

void TextRenderer::bufferSetup(){
    // set up vao, vbo for the quad on which we render the glyph bitmap
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // each vertex has 4 flaot, and we need 6 vertices for a quad
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    // set attribute in location 0, which is the in-vertex for vertex shader. See the definition in (TextRenderProgram, line.13)
    glEnableVertexAttribArray(0);
    // tells vao how to read from buffer. (read 4 floats each time, which is one vertex)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    // done
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);    
}