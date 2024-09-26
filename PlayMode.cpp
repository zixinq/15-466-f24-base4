#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextureProgram.hpp"

/*
 #include "hello-harfbuzz-freetype.hpp"
#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include <GL/gl.h>
#include <glm/gtc/type_ptr.hpp>
 */

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "load_save_png.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>

#define GL_CHECK_ERROR() { \
    GLenum err; \
    while ((err = glGetError()) != GL_NO_ERROR) { \
        std::cerr << "OpenGL error: " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
    } \
}

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
    MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
    hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    return ret;
});


Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
    
    return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
        Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;

        drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;

    });
    
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
    return new Sound::Sample(data_path("dusty-floor.opus"));
});

void PlayMode::loadFont(const std::string& fontPath) {
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not initialize FreeType Library" << std::endl;
        return;
    }

    // Load the font face
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    // Set the size of glyphs
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction for OpenGL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load the first 128 ASCII characters
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph for character: " << (int)c << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character information
        Character character = { texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), face->glyph->advance.x };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    // Clean up FreeType objects
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}


void checkOpenGLError(const std::string& location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
    }
}

void PlayMode::initializeBuffers() {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
         //upload texture data:
        //load texture data from a file:
        std::vector< glm::u8vec4 > data;
        glm::uvec2 size;
        load_png(data_path("out.png"), &size, &data, LowerLeftOrigin);
    
        if (size.x == 0 || size.y == 0) {
            std::cerr << "Failed to load texture data" << std::endl;
        }
    
        std::cout << textureID << "\n";
    
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Mipmaps (optional)
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    
        GLfloat vertices[] = {
            // Positions          // Texture Coords
            0.5f,  0.5f, 0.0f,   1.0f, 1.0f,  // Top right
            0.5f, -0.5f, 0.0f,   1.0f, 0.0f,  // Bottom right
           -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,  // Top left
           -0.5f, -0.5f, 0.0f,   0.0f, 0.0f   // Bottom left
        };

        // Generate and bind VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Set vertex attribute pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);              // Position
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));  // Texture Coords
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);  // Unbind VAO
            /*
    
        
                     
            
    
        GLuint VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
    
        GLfloat vertices[] = {
            // Positions (X, Y, Z, W)
            -0.5f, -0.5f, 0.0f, 1.0f,  // Lower-left
             0.5f, -0.5f, 0.0f, 1.0f,  // Lower-right
             0.0f,  0.5f, 0.0f, 1.0f   // Top
        };

        // Generate and bind a Vertex Array Object (VAO)
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Generate and bind a Vertex Buffer Object (VBO)
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Specify the layout of the vertex data
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);  // Position attribute
        glEnableVertexAttribArray(0);

        // Unbind the VBO and VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        GLfloat vertices[] = {
            // Positions          // Texture Coords
             0.5f,  0.5f,  1.0f, 1.0f,
             0.5f, -0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.0f, 1.0f
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        checkOpenGLError("VAO/VBO Initialization");

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        // Check if attribute pointer is correctly set
        GLint attribEnabled;
        glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled);
        if (!attribEnabled) {
            std::cerr << "Vertex attribute 0 is not enabled!" << std::endl;
        }
        GL_CHECK_ERROR(); // Check for errors after setting up attributes
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
     */
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
    // Get pointers to leg for convenience:
    for (auto &transform : scene.transforms) {
        if (transform.name == "Hip.FL") hip = &transform;
        else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
        else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
    }
    if (!hip || !upper_leg || !lower_leg) throw std::runtime_error("Could not find necessary leg components.");
    
    hip_base_rotation = hip->rotation;
    upper_leg_base_rotation = upper_leg->rotation;
    lower_leg_base_rotation = lower_leg->rotation;
    
    // Get pointer to the camera:
    if (scene.cameras.size() != 1) throw std::runtime_error("Expected one camera in the scene.");
    camera = &scene.cameras.front();
    
    leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
    
    /*
     //tex_example
     glGenTextures(1, &tex_example.tex);
     { //upload texture data:
     
     //load texture data as RGBA from a file:
     std::vector< glm::u8vec4 > data;
     glm::uvec2 size;
     load_png(data_path("out.png"), &size, &data, LowerLeftOrigin);
     
     glBindTexture(GL_TEXTURE_2D, tex_example.tex);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
     glGenerateMipmap(GL_TEXTURE_2D);
     glBindTexture(GL_TEXTURE_2D, 0);
     }
     
     //create name for vertex buffer:
     glGenBuffers(1, &tex_example.tristrip);
     //(will upload data later)
     
     { //set up vertex array:
     glGenVertexArrays(1, &tex_example.tristrip_for_texture_program);
     glBindVertexArray(tex_example.tristrip_for_texture_program);
     glBindBuffer(GL_ARRAY_BUFFER, tex_example.tristrip);
     
     glVertexAttribPointer( texture_program->Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(PosTexVertex), (GLbyte *)0 + offsetof(PosTexVertex, Position) );
     glEnableVertexAttribArray( texture_program->Position_vec4 );
     
     glVertexAttribPointer( texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(PosTexVertex), (GLbyte *)0 + offsetof(PosTexVertex, TexCoord) );
     glEnableVertexAttribArray( texture_program->TexCoord_vec2 );
     
     glBindBuffer(GL_ARRAY_BUFFER, 0);
     glBindVertexArray(0);
     
     }
     */
    
    /*
     // Initialize FreeType library
     if (FT_Init_FreeType(&ft)) {
     std::cerr << "ERROR::FREETYPE: Could not initialize FreeType Library" << std::endl;
     }
     
     // Load the font face
     if (FT_New_Face(ft, "GentiumBookPlus-Bold.ttf", 0, &face)) {
     std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
     }
     
     // Set the size of the glyphs
     FT_Set_Pixel_Sizes(face, 0, 48); // Set the font size to 48px height
     
     // Disable byte-alignment restriction for textures
     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
     // Load the first 128 ASCII characters
     for (unsigned char c = 0; c < 128; c++) {
     // Load the character glyph
     if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
     std::cerr << "ERROR::FREETYPE: Failed to load Glyph for character: " << c << std::endl;
     continue;
     }
     
     // Generate texture for the glyph
     GLuint texture;
     glGenTextures(1, &texture);
     glBindTexture(GL_TEXTURE_2D, texture);
     glTexImage2D(
     GL_TEXTURE_2D,
     0,
     GL_RED,
     face->glyph->bitmap.width,
     face->glyph->bitmap.rows,
     0,
     GL_RED,
     GL_UNSIGNED_BYTE,
     face->glyph->bitmap.buffer
     );
     
     GLenum error = glGetError();
     if (error != GL_NO_ERROR) {
     std::cerr << "Error generating texture: " << error << "\n";
     }
     
     // Set texture options
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     
     // Store character information for rendering
     Character character = {
     texture,
     glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
     glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
     static_cast<GLuint>(face->glyph->advance.x)
     };
     Characters.insert(std::pair<char, Character>(c, character));
     }
     
     FT_Done_Face(face);
     FT_Done_FreeType(ft);
     // Unbind the texture
     glBindTexture(GL_TEXTURE_2D, 0);
     
     */
    
    //loadFont("GentiumBookPlus-Bold.ttf");
    initializeBuffers();

    /*
    
    // load font
    // Initialize FreeType library
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not initialize FreeType Library" << std::endl;
        return;
    }
    
    // Load the font face
    if (FT_New_Face(ft, "GentiumBookPlus-Bold.ttf", 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }
    
    // Set the size of the glyphs
    FT_Set_Pixel_Sizes(face, 0, 62);  // Set the font size to 48px height
    
    // Disable byte-alignment restriction for textures
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    for (unsigned char c = 0; c < 128; c++) {
        // Load the character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph for character: " << c << std::endl;
            continue;
        }

        // Check if the glyph has valid dimensions
        if (face->glyph->bitmap.width == 0 || face->glyph->bitmap.rows == 0) {
            // If the glyph is empty, insert an empty character
            Character character = {0, glm::ivec2(0, 0), glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), face->glyph->advance.x};
            characters.push_back(character);
            continue;
        }

        // Generate a texture for the glyph
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Error after glBindTexture: " << error << std::endl;
        }

        // Upload the glyph bitmap to the texture
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,                      // Only need a single channel (greyscale)
            face->glyph->bitmap.width,   // Width of the glyph
            face->glyph->bitmap.rows,    // Height of the glyph
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer   // The glyph's bitmap data
        );

        // Check for OpenGL errors during texture creation
        error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Error generating texture: " << error << "\n";
        }

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character information for rendering
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        characters.push_back(character);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    
    GLenum err2 = glGetError();
    if (err2 != GL_NO_ERROR) {
        std::cerr << "Error after glBindVertexArray: " << err2 << std::endl;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Allocate buffer space for 6 vertices (2 triangles to form a quad)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    // Define vertex attributes (position + texture coordinates)
    // Assuming that "Position_vec4" is location 0, and "TexCoord_vec2" is location 1
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);  // Position and texture coords
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Check if VAO and VBO were generated successfully
    if (VAO == 0 || VBO == 0) {
        std::cerr << "VAO/VBO error: Could not create VAO or VBO." << "\n";
        return;
    }
     */
}


PlayMode::~PlayMode() {
}


void PlayMode::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    // Set text color
    glUniform3f(glGetUniformLocation(texture_program->program, "textColor"), color.x, color.y, color.z);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each character
    for (const char& c : text) {
        Character ch = Characters[c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Vertices for the glyph quad
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        // Bind texture for the current glyph
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor for the next glyph
        x += (ch.Advance >> 6) * scale;  // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    // Clean up state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
    glUseProgram(0); // Unbind shader program
}


/*
void PlayMode::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    GLint colorLocation = glGetUniformLocation(texture_program->program, "textColor");
    if (colorLocation == -1) {
        std::cerr << "Uniform 'textColor' not found!" << std::endl;
        return;
    }
    glUniform3f(colorLocation, color.r, color.g, color.b);


    if (texture_program->CLIP_FROM_LOCAL_mat4 != -1) {
        glUniformMatrix4fv(texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(0.1f)));
    } else {
        std::cerr << "ERROR: 'CLIP_FROM_LOCAL' uniform not found in shader." << std::endl;
    }
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Error setting uniform: " << error << std::endl;
    }
    
    GLint clipFromLocalLoc = glGetUniformLocation(texture_program->program, "CLIP_FROM_LOCAL");
    GLint texLoc = glGetUniformLocation(texture_program->program, "TEX");
    GLint textColorLoc = glGetUniformLocation(texture_program->program, "textColor");

    if (clipFromLocalLoc == -1 || texLoc == -1 || textColorLoc == -1) {
        std::cerr << "ERROR: Uniform not found!" << std::endl;
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    
    
    
   //std::string::const_iterator c;
    
  for (unsigned int i = 0; i < characters.size();i++){
            Character ch = characters[i];
            
            
            GLfloat xpos = x + ch.Bearing.x * scale;
            GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            GLfloat w = ch.Size.x * scale;
            GLfloat h = ch.Size.y * scale;
      
            GLfloat vertices[] = {
                  // Positions for a simple triangle
                  -0.5f, -0.5f, 0.0f, 1.0f,  // Lower-left
                   0.5f, -0.5f, 0.0f, 1.0f,  // Lower-right
                   0.0f,  0.5f, 0.0f, 1.0f   // Top
              };

              // Bind VBO and upload data
              glBindBuffer(GL_ARRAY_BUFFER, VBO);
              glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

              // Set up vertex attributes
              glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
              glEnableVertexAttribArray(0);
            
            // Create vertex data for the quad
            GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            
            // Bind glyph texture
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            error = glGetError();
            if (error != GL_NO_ERROR) {
                  std::cerr << "Error after glBindTexture: " << error << std::endl;
            }
            
            GL_CHECK_ERROR();
      
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            // Draw the quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
      
            error = glGetError();
            if (error != GL_NO_ERROR) {
                  std::cerr << "Error after glDrawArrays: " << error << std::endl;
            }
            // Advance the cursor for the next glyph
            x += (ch.Advance >> 6) * scale;
    
        
        
    }
    // Unbind VAO and VBO after rendering
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    
        GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL Error: " << err << " during RenderText." << std::endl;
            }
     
    
    
}
*/


bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_ESCAPE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            return true;
        } else if (evt.key.keysym.sym == SDLK_a) {
            left.downs += 1;
            left.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_d) {
            right.downs += 1;
            right.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_w) {
            up.downs += 1;
            up.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_s) {
            down.downs += 1;
            down.pressed = true;
            return true;
        }
    } else if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_a) {
            left.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_d) {
            right.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_w) {
            up.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_s) {
            down.pressed = false;
            return true;
        }
    } else if (evt.type == SDL_MOUSEBUTTONDOWN) {
        if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            return true;
        }
    } else if (evt.type == SDL_MOUSEMOTION) {
        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
            glm::vec2 motion = glm::vec2(
                evt.motion.xrel / float(window_size.y),
                -evt.motion.yrel / float(window_size.y)
            );
            camera->transform->rotation = glm::normalize(
                camera->transform->rotation
                * glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
                * glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
            );
            return true;
        }
    }

    return false;
}



void PlayMode::update(float elapsed) {

    //slowly rotates through [0,1):
    wobble += elapsed / 10.0f;
    wobble -= std::floor(wobble);

    hip->rotation = hip_base_rotation * glm::angleAxis(
        glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
        glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
        glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    //move sound to follow leg tip position:
    leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);
    

    
    //move camera:
    {

        //combine inputs into a move:
        constexpr float PlayerSpeed = 30.0f;
        glm::vec2 move = glm::vec2(0.0f);
        if (left.pressed && !right.pressed) move.x =-1.0f;
        if (!left.pressed && right.pressed) move.x = 1.0f;
        if (down.pressed && !up.pressed) move.y =-1.0f;
        if (!down.pressed && up.pressed) move.y = 1.0f;

        //make it so that moving diagonally doesn't go faster:
        if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

        glm::mat4x3 frame = camera->transform->make_local_to_parent();
        glm::vec3 frame_right = frame[0];
        //glm::vec3 up = frame[1];
        glm::vec3 frame_forward = -frame[2];

        camera->transform->position += move.x * frame_right + move.y * frame_forward;
    }

    { //update listener to camera position:
        glm::mat4x3 frame = camera->transform->make_local_to_parent();
        glm::vec3 frame_right = frame[0];
        glm::vec3 frame_at = frame[3];
        Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
    }

    //reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;
    /*
    { //texture example
        std::vector< PosTexVertex > verts;
        //lower left
        verts.emplace_back(PosTexVertex{
            .Position = glm::vec3(0.0f, 0.0f, 0.0f),
            .TexCoord = glm::vec2(0.0f, 0.0f),
        });
        //upper left
        verts.emplace_back(PosTexVertex{
            .Position = glm::vec3(0.0f, 1.0f, 0.0f),
            .TexCoord = glm::vec2(0.0f, 1.0f),
        });
        verts.emplace_back(PosTexVertex{
            .Position = glm::vec3(1.0f, 0.0f, 0.0f),
            .TexCoord = glm::vec2(1.0f, 0.0f),
        });
        verts.emplace_back(PosTexVertex{
            .Position = glm::vec3(1.0f, 1.0f, 0.0f),
            .TexCoord = glm::vec2(1.0f, 1.0f),
        });
        
        glBindBuffer(GL_ARRAY_BUFFER, tex_example.tristrip);
        glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(verts[0]), verts.data(), GL_STREAM_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        tex_example.count = verts.size();
        
        tex_example.CLIP_FROM_LOCAL = glm::mat4(1.0f);
    }
     
    GL_ERRORS();
    */
}

void drawText(GLuint VAO) {
    
    
    glBindVertexArray(VAO);
    checkOpenGLError("VAO Bind");

    // Issue the draw call
    //glDrawArrays(GL_TRIANGLES, 0, 6);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at Draw Call: " << error << std::endl;
    }

    glBindVertexArray(0);
    checkOpenGLError("Unbind VAO");
    glBindTexture(GL_TEXTURE_2D, 0);
}


void PlayMode::draw(glm::uvec2 const &drawable_size) {
    //update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);

/*
    //set up light type and position for lit_color_texture_program:
    // TODO: consider using the Light(s) in the scene to do this
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);

    
    scene.draw(*camera);
 

    GL_ERRORS();
    glClearColor(0.8f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(drawable_size.x), 0.0f, static_cast<float>(drawable_size.y));
    glUniformMatrix4fv(glGetUniformLocation(texture_program->program, "CLIP_FROM_LOCAL"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    */
    glUseProgram(texture_program->program);
    checkOpenGLError("Use Shader Program");
    /*

    // Set up an orthographic projection for rendering text in 2D
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(drawable_size.x), 0.0f, static_cast<float>(drawable_size.y));
    
    GLuint projectionLoc = glGetUniformLocation(texture_program->program, "CLIP_FROM_LOCAL");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    //glUniformMatrix4fv(glGetUniformLocation(texture_program->program, "CLIP_FROM_LOCAL"), 1, GL_FALSE, glm::value_ptr(projection));
    checkOpenGLError("Set Projection Matrix");

    // Disable depth testing for 2D rendering and enable blending for transparent text
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
    
    
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(texture_program->program, "texture1"), 0);

    // Bind the VAO and draw the quad
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  // GL_TRIANGLE_STRIP for rendering a quad as two triangles

    // Unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    
    //drawText(VAO);

    // Render some text
    //RenderText("Hello, World!", 0.0f, 0.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    // Check for OpenGL errors
    
    /*
    GLint position_loc = glGetAttribLocation(texture_program->program, "Position");

    // Check if locations are valid
    if (position_loc == -1) {
        std::cerr << "ERROR: 'Position' attribute not found in shader." << std::endl;
    } else {
        std::cout << "'Position' attribute location: " << position_loc << std::endl;
    }
  
 
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "Error after glUseProgram: " << err << std::endl;
    }
    
    glUniform1i(glGetUniformLocation(texture_program->program, "TEX"), 0);
*/
    

    //RenderText("Hello, World!", 0.1f , 0.1f, 0.1f, glm::vec3(1.0f, 1.0f, 1.0f));


    glUseProgram(0);
 
    /*
    
    glClearColor(0.8f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glBindVertexArray(tex_example.tristrip_for_texture_program);
    glUseProgram(texture_program->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_example.tex);

    glUniformMatrix4fv( texture_program->CLIP_FROM_LOCAL_mat4, 1, GL_FALSE, glm::value_ptr(tex_example.CLIP_FROM_LOCAL) );

    glDrawArrays(GL_TRIANGLE_STRIP, 0, tex_example.count);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glBindVertexArray(0);
     */
    
    GL_ERRORS();
    
}

glm::vec3 PlayMode::get_leg_tip_position() {
    //the vertex position here was read from the model in blender:
    return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
