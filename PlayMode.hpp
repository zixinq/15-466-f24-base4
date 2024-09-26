#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "TextureProgram.hpp"

#include <map>
#include <glm/glm.hpp>

#include <vector>
#include <deque>


#include <ft2build.h>
#include FT_FREETYPE_H


struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
    

	//----- game state -----
    struct PosTexVertex {
        glm::vec3 Position;
        glm::vec2 TexCoord;
    };
    
    static_assert( sizeof(PosTexVertex) == 3*4 + 2*4, "PosTexVertex is packed." );
    struct {
        GLuint tex = 0; //created at startup
        GLuint tristrip = 0; //vertex buffer (of PosTexVertex)
        GLuint tristrip_for_texture_program = 0; //vertex array object

        GLuint count = 0; //number of vertices in buffer
        glm::mat4 CLIP_FROM_LOCAL = glm::mat4(1.0f); //transform to use when drawing
    } tex_example;
    /*
    struct{
        GLuint tex = 0;
        GLuint VBO = 0; //buffer
        GLuint VAO = 0; //vao
        
        GLuint count = 0; //how many things in buffer
        glm::mat4 CLIP_FROM_LOCAL = glm::mat4(1.0f);
    }text_renderer;
     */

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, one, two;
    
    struct Character {
        GLuint TextureID;  // ID handle of the glyph texture
        glm::ivec2 Size;   // Size of glyph
        glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
        long Advance;      // Offset to advance to next glyph
    };
    std::map<char, Character> Characters;  // Store glyphs for text rendering
    
    FT_Library ft;
    FT_Face face;
    
    
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color);
    void loadFont(const std::string& fontPath);
    void initializeBuffers();
    
    
    GLuint textureID;
    GLuint VBO, VAO;
    
    struct Story {
        std::string text;
        std::string choice1;
        std::string choice2;
        int nextState1;
        int nextState2;
        
    };
    
    std::vector<Story> story;
    int currentState;
    
    int handle_choice(int currentState, int choice, const std::vector<Story>& story);
   
	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;

	glm::vec3 get_leg_tip_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};


