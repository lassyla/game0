#include "BobMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

BobMode::BobMode() {
	//create initial heads
    heads.emplace_back(glm::vec2(-2.5f, 0.0f), default_hair_length); 
    heads.emplace_back(glm::vec2(0.0f, 0.0f), default_hair_length); 
    heads.emplace_back(glm::vec2(2.5f, 0.0f), default_hair_length); 
    heads.emplace_back(glm::vec2(5.0f, 0.0f), default_hair_length); 
	heads.at(1).velocity = glm::vec2(0.0f, max_head_speed); 
    heads.at(1).visible = true; 
	heads.at(3).velocity = glm::vec2(0.0f, -max_head_speed); 
    heads.at(3).visible = true; 

	num_visible = 1; 

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of BobMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

BobMode::~BobMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool BobMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);
        glm::vec2 court_mouse = clip_to_court  * glm::vec3(clip_mouse, 1.0f);
        if(!knife_thrown) knife_angle = atan2(court_mouse.y - knife.y, court_mouse.x - knife.x); 
        else new_knife_angle = atan2(court_mouse.y - knife.y, court_mouse.x - knife.x); 
	}
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
        knife_thrown = true; 
	}

	return false;
}

void BobMode::update(float elapsed) {
    
    //update knife position 
    if(knife_thrown) {
        knife = knife + knife_speed * glm::vec2(cos(knife_angle), sin(knife_angle)); 
        //reset knife if it goes offscreen
        if(knife.x + knife_radius.x < - court_radius.x 
        || knife.y + knife_radius.y < - court_radius.y
        || knife.x - knife_radius.x > court_radius.x
        || knife.y - knife_radius.y > court_radius.y){
            knife_thrown = false; 
            knife_angle = new_knife_angle; 
            knife = knife_start; 
        }
    }

    if(lives == 0) return; 
    //spawn new heads 
	if(add_heads > 0 && num_visible < num_heads) {
		add_elapsed += elapsed; 
		if(add_elapsed > reappear_time) {
			int randIndex = int(std::rand() * 1.0f / RAND_MAX * (num_heads - num_visible));

			for (std::vector<Head>::iterator  head = std::begin(heads); head != std::end(heads); ++head) {
				if(!head->visible) {
					if(randIndex == 0) {
						head->visible = true; 
						num_visible++; 
						add_heads--; 
						add_elapsed = 0.0f; 
						head->velocity.y = max_head_speed; 
						if(std::rand() / RAND_MAX < 0.5) head->velocity.y *= -1; 
						head->hair_length = default_hair_length; 
						head->happiness = -1; 
						head->dead = false; 
						head->hair_angle = 0; 
						head->position.y = court_radius.y - std::rand() * 1.0f / RAND_MAX * court_radius.y * 2; 
					} 
					randIndex--;
				}
			}

		}
	}

    //update head position 
    for (std::vector<Head>::iterator  head = std::begin(heads); head != std::end(heads); ++head) {
    //for(int i = 0; i < heads.length; i++) {
        //Head head = heads[i]; 
        if(!head->visible) continue; 

        //head disappearance after killed 
        if(head->dead) {
            head->vis_elapsed += elapsed; 
            if(head->vis_elapsed >  disappear_time) {
				head->visible = false; 
				num_visible--; 
			}
            continue; 
        }
        if(head->happiness > happy_threshold) {
            head->vis_elapsed += elapsed; 
            if(head->vis_elapsed >  disappear_time) {
				head->visible = false; 
				num_visible--; 
			}
            continue; 
        }
        head->position += elapsed * head->velocity; 
        //bounce off top and bottom 
        if(head->position.y - head_radius.y < - court_radius.y){
            head->velocity.y = abs(head->velocity.y); 
        }
        if(head->position.y + head_radius.x > court_radius.y) {
            head->velocity.y = abs(head->velocity.y) * -1; 
        }

        head->cut_elapsed += elapsed; 
        //check for collision with knife  point horizontally
        if(head->cut_elapsed > cut_time 
        &&knife.x + knife_radius.x * cos(knife_angle) > head->position.x - head_radius.x
        && knife.x + knife_radius.x * cos(knife_angle) < head->position.x + head_radius.x) {
			//if it is below the top of the head
            if(knife.y + knife_radius.x * sin(knife_angle) < head->position.y + head_radius.y) {
                //if it hits the head
                if(knife.y + knife_radius.x * sin(knife_angle) > head->position.y - head_radius.y) {
                    head->dead = true; 
                    head->vis_elapsed = 0.0f; 
                    head->happiness = -1;
                    lives -= 1; 
					add_heads++; 
                }
                //if it hits the hair 
                else if(knife.y + knife_radius.x * sin(knife_angle) > head->position.y - head_radius.y - head->hair_length) {
                    float old_length = head->hair_length; 
                    head->hair_length = std::max(0.01f, head->position.y - head_radius.y - (knife.y + .5f * knife_radius.x * sin(knife_angle))); 
                    head->hair_angle = knife_angle; 
                    head->happiness = 1.0f - head->hair_length / default_hair_length * 2.0f; 

                    head->cut_elapsed = 0.0f; 
					if(head->velocity.y > 0){
						head->velocity.y = min_head_speed + (max_head_speed - min_head_speed) * (1.0f - head->happiness) / 2.0f;
					} 
                    //score += old_length - head->hair_length; 
                    if (head->happiness > happy_threshold) {
                        head->vis_elapsed = 0.0f; 
                        score++; 
						max_head_speed = std::max(2.0f + score / 5.0f, 6.0f); 
						add_heads ++; 
                    }
                }
            }
        }
    }
}

void BobMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
    glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x171714ff);
    if(lives == 0) bg_color = HEX_TO_U8VEC4(0xaa3333ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xffffaaff);
	const glm::u8vec4 heart_color = HEX_TO_U8VEC4(0xff7777ff);
	const glm::u8vec4 hair_color = HEX_TO_U8VEC4(0x604d29ff);
	const glm::u8vec4 dead_color = HEX_TO_U8VEC4(0x777777ff);
    const std::vector< glm::u8vec4 > head_colors = {
		HEX_TO_U8VEC4(0xff7777ff), HEX_TO_U8VEC4(0xeb7d34cff), HEX_TO_U8VEC4(0xebb434ff),
		HEX_TO_U8VEC4(0xf5e536ff), HEX_TO_U8VEC4(0xcff03eff), HEX_TO_U8VEC4(0x92f041ff)
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

    //adapted from https://gist.github.com/linusthe3rd/803118
    auto draw_circle = [&vertices](glm::vec2 const &center, float radius, float start_angle, float angle_elapsed, glm::u8vec4 const &color){
        int num_points = 20; //# of triangles used to draw full circle
        
        float angle = angle_elapsed/ num_points;
        
        for(int i = 0; i < num_points;i++) { 
            vertices.emplace_back(glm::vec3(center.x, center.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
            vertices.emplace_back(glm::vec3(center.x + (radius * cos(start_angle + i *  angle)), center.y + (radius * sin(start_angle + i * angle)), 0.0f), color, glm::vec2(0.5f, 0.5f));
            vertices.emplace_back(glm::vec3(center.x + (radius * cos(start_angle + (i+1) *  angle)), center.y + (radius * sin(start_angle + (i+1) * angle)), 0.0f), color, glm::vec2(0.5f, 0.5f));
        }
    };

	//inline helper function for quad drawing:
	auto draw_quad = [&vertices](glm::vec2 const &p1, glm::vec2 const &p2, glm::vec2 const &p3, glm::vec2 const &p4, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(p1.x, p1.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(p2.x, p2.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(p3.x, p3.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(p1.x, p1.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(p3.x, p3.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(p4.x, p4.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//inline helper function for rectangle drawing with rotation:
	auto draw_rectangle_rot = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, float angle, glm::u8vec4 const &color) {
        //transformation matrix, explanation at https://open.gl/transformations
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::translate(trans, glm::vec3(center.x, center.y, 0.0f)); 
        trans = glm::rotate(trans, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        trans = glm::translate(trans, glm::vec3(-center.x, -center.y, 0.0f));

		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x-radius.x, center.y-radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x+radius.x, center.y-radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x+radius.x, center.y+radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x-radius.x, center.y-radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x+radius.x, center.y+radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(trans * glm::vec4(center.x-radius.x, center.y+radius.y, 0.0f, 1.0f)), color, glm::vec2(0.5f, 0.5f));
	};

    //heads
    glm::vec2 eye_radius = glm::vec2(0.1f, 0.1f); 
    glm::vec2 nose_radius = glm::vec2(0.05f, 0.05f); 
    glm::vec2 nose_position = glm::vec2(0.00f, -0.05f); 
    glm::vec2 x_radius = glm::vec2(0.2f, 0.05f); 
    glm::vec2 left_eye_position = glm::vec2(-.2f, .1f); 
    glm::vec2 right_eye_position = glm::vec2(.2f, .1f); 
    glm::vec2 mouth_position = glm::vec2(0.0f, -.25f); 
    glm::vec2 left_ear_position = glm::vec2(-.5f, 0.0f); 
    glm::vec2 right_ear_position = glm::vec2(.5f, 0.0f); 

    for (std::vector<Head>::iterator  head = std::begin(heads); head != std::end(heads); ++head) {
            if(!head->visible) continue; 
        	//hair points 
            glm::vec2 p1 = glm::vec2(head->position.x - head_radius.x, head->position.y ); 
            glm::vec2 p2 = glm::vec2(head->position.x + head_radius.x, head->position.y ); 
            glm::vec2 p3 = glm::vec2(head->position.x + head_radius.x, head->position.y - head_radius.y - head->hair_length + head_radius.x * sin(head->hair_angle)); 
            glm::vec2 p4 = glm::vec2(head->position.x - head_radius.x, head->position.y - head_radius.y - head->hair_length);

            draw_quad(p1, p2, p3, p4, hair_color);

            //round part of hair 
            draw_circle(head->position, 0.8f, 0.0f, 3.14f, hair_color);

            //draw head
        	if(head->dead || lives==0) {
                draw_circle(head->position, 0.65f, 0.0f, 2.0f * 3.142f, dead_color);
                draw_circle(head->position + left_ear_position, 0.25f, 0.0f, 2.0f * 3.142f, dead_color);
                draw_circle(head->position + right_ear_position, 0.25f, 0.0f, 2.0f * 3.142f,dead_color);

                //draw face
                draw_rectangle_rot(head->position + left_eye_position, x_radius, .79f, hair_color); 
                draw_rectangle_rot(head->position + right_eye_position, x_radius, .79f,hair_color); 
                draw_rectangle_rot(head->position + left_eye_position, x_radius, -.79f, hair_color); 
                draw_rectangle_rot(head->position + right_eye_position, x_radius, -.79f,hair_color); 
                draw_rectangle(head->position + nose_position, nose_radius, hair_color); 
                draw_rectangle(head->position + mouth_position, glm::vec2(0.45f, 0.1f), hair_color);                
            }
            else {
                //draw_rectangle(head->position, head_radius, head_colors[int(head_colors.size() * .5 * (head->happiness + 1))]);
                int color_index = int(head_colors.size() * .5f * (head->happiness + 1.0f)); 
                draw_circle(head->position, 0.65f, 0.0f, 2.0f * 3.142f, head_colors[color_index]);
                draw_circle(head->position + left_ear_position, 0.25f, 0.0f, 2.0f * 3.142f, head_colors[color_index]);
                draw_circle(head->position + right_ear_position, 0.25f, 0.0f, 2.0f * 3.142f, head_colors[color_index]);


                //draw face
                draw_rectangle(head->position + left_eye_position, eye_radius, hair_color); 
                draw_rectangle(head->position + right_eye_position, eye_radius, hair_color); 
                draw_rectangle(head->position + nose_position, nose_radius, hair_color); 
                draw_rectangle(head->position + mouth_position, glm::vec2(0.4f, 0.05f), hair_color); 
                draw_rectangle(head->position + mouth_position + glm::vec2(0.4f, head->happiness * 0.05f), glm::vec2(0.05f, head->happiness * 0.1f), hair_color); 
                draw_rectangle(head->position + mouth_position + glm::vec2(-0.4f, head->happiness * 0.05f), glm::vec2(0.05f, head->happiness * 0.1f), hair_color); 
            }
    }


    //knife
	draw_rectangle_rot(knife, knife_radius, knife_angle, fg_color);

	//scores:
	glm::vec2 life_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < lives; ++i) {
		draw_rectangle(glm::vec2( court_radius.x - (2.0f + 3.0f * i) * life_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * life_radius.y), life_radius, heart_color);
	}
    for (uint32_t i = 0; i < score; ++i) {
		draw_rectangle(glm::vec2( - court_radius.x + (2.0f + 3.0f * i) * life_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * life_radius.y), life_radius, head_colors[5]);
	}

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * life_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);


	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
