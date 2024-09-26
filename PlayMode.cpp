#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TextureProgram.hpp"

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

//the shader and font loading is adapted from https://learnopengl.com/In-Practice/Text-Rendering


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

    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph for character: " << (int)c << std::endl;
            continue;
        }

        // Manually add space
        if (face->glyph->bitmap.width == 0 || face->glyph->bitmap.rows == 0) {
            Character ch = {
                0,
                glm::ivec2(0, 0),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, ch));
        } else {
            //generate texture
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                         0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

     
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

          
            Character ch = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, ch));
        }
    }

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
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

   
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
    
    //leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
   
    
    loadFont("GentiumBookPlus-Bold.ttf");
    initializeBuffers();

    story = {
            {"Do you like red or yellow?", "Red", "Yello", 1, 2},
            {"Do you like apple or strawberry?", "Apple", "Strawberry", 3, 4},
            {"Do you like banana or mango", "Banana", "Mango", 5, 6},
            {"You like apple!", "", "", -1, -1},
            {"You like Strawberry!", "", "", -1, -1},
            {"You like banana!", "", "", -1, -1},
            {"You like mango!", "", "", -1, -1}
        };
    currentState = 0;
}


PlayMode::~PlayMode() {
}


void PlayMode::RenderText(const std::string &text, float x, float y, float scale, glm::vec3 color) {
    glUniform3f(glGetUniformLocation(texture_program->program, "textColor"), color.x, color.y, color.z);
  

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];
        
        if (*c == ' ') {
            x += (ch.Advance >> 6) * scale;
            continue;
        }


        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
       
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

       
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
      
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
       
        glDrawArrays(GL_TRIANGLES, 0, 6);

        
        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}



int PlayMode::handle_choice(int currentState, int choice, const std::vector<Story>& story) {
    if (choice == 1) {
        return story[currentState].nextState1;
    } else if (choice == 2) {
        return story[currentState].nextState2;
    } else {
        std::cout << "Invalid Option." << std::endl;
        return currentState;
    }
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_1) {
            one.pressed = true;
            currentState = handle_choice(currentState, 1, story);
            return true;
        } else if (evt.key.keysym.sym == SDLK_2) {
            two.pressed = true;
            currentState = handle_choice(currentState, 2, story);
            return true;
        }
    } else if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_1) {
            one.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_d) {
            two.pressed = false;
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
    //leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);
        
    if (currentState == -1) {
        std::cout << "The story has ended. Press ESC to quit." << std::endl;
    }
    
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

void PlayMode::draw_story(const Story &currentStory) {
    std::cout << currentStory.text << std::endl;
    if (!currentStory.choice1.empty()) {
        std::cout << "1. " << currentStory.choice1 << std::endl;
    }
    if (!currentStory.choice2.empty()) {
        std::cout << "2. " << currentStory.choice2 << std::endl;
    }
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
    
    */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(texture_program->program);
    glUniform3f(texture_program->textColor_vec3, 1.0f, 1.0f, 1.0f);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(drawable_size.x),
                                      0.0f, static_cast<float>(drawable_size.y));
    glUniformMatrix4fv(glGetUniformLocation(texture_program->program, "CLIP_FROM_LOCAL"), 1, GL_FALSE, glm::value_ptr(projection));
    
    Story &node = story[currentState];

    RenderText(node.text, 100.0f,1000.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    RenderText("1: " + node.choice1, 100.0f, 950.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    RenderText("2: " + node.choice2, 100.0f, 900.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
   
   
    // Render text
    //RenderText("Hello, World!", 25.0f, 25.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    GL_ERRORS();
    
}

glm::vec3 PlayMode::get_leg_tip_position() {
    //the vertex position here was read from the model in blender:
    return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
