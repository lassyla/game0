#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * BobMode is a game mode that implements a single-player game of Pong.
 */

struct BobMode : Mode {
	BobMode();
	virtual ~BobMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);

    glm::vec2 knife_start = glm::vec2(-6.5f, 0.0f);
    glm::vec2 knife = glm::vec2(-6.5f, 0.0f);
	glm::vec2 knife_radius = glm::vec2(1.0f, .05f);
    float knife_angle = 0.0f; 
    float new_knife_angle = 0.0f; 
    bool knife_thrown = false; 
    float knife_speed = 1.0f; 

	uint32_t lives = 3;
	uint32_t score = 0;
    //float score = 0.0f; 

    float default_hair_length = 2.0f; 
    float disappear_time = 1.0f; 
    float reappear_time = 1.5f; 
    float happy_threshold = 0.66f; 
    float cut_time = 0.4f; 

    int num_visible = 0;
    int num_heads = 4;
    int add_heads = 0; 
    float add_elapsed = 0.0f; 

    glm::vec2 head_radius = glm::vec2(0.8f, 0.8f);
    float min_head_speed = .5f; 
    float max_head_speed = 2.0f; 

    struct Head {
        glm::vec2 position; 
        glm::vec2 velocity; 
        bool dead; 
        bool visible; 
        float hair_angle; 
        float hair_length; 
        //ranges from -1 to 1
        float happiness; 
        float vis_elapsed; 
        float cut_elapsed; 
        Head(glm::vec2 const &pos, float const &len) : 
            position(pos), hair_length(len)
        { 
            dead = false; 
            visible = false; 
            hair_angle = 0; 
            happiness = -1; 
            velocity = glm::vec2(0.0f, 0.0f); 
        }
    };
	std::vector<Head> heads;

    struct Hair {
        glm::vec2 position; 
        glm::vec2 velocity; 
        float length; 
        float top_angle;
        float bottom_angle; 
        Hair(glm::vec2 const &pos, glm::vec2 const &vel, float const &len, float const &top, float const &bot) : 
            position(pos), velocity(vel), length(len), top_angle(top), bottom_angle(bot) {}
    };
	std::vector<Hair> hairs;


	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "BobMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
