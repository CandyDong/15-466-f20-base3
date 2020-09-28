#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <math.h>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("scene.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("scene.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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

Load< Sound::Sample > zombie_sample_1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("zombie_1.opus"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Zombie") player = &transform;
		if (transform.name == "Zombie.001" || transform.name == "Zombie.002" ||
			transform.name == "Zombie.003" || transform.name == "Zombie.004") {
			// the Tile pointer is to be populated in initialize_board
			Entity *zombie = new Entity(&transform, Character::zombie);
			zombies.emplace_back(zombie);
		}
		if (transform.name == "grandma" || transform.name == "grandma.001" ||
			transform.name == "grandma.002" || transform.name == "grandma.003" ||
			transform.name == "grandma.004") {
			Entity *human = new Entity(&transform, Character::human);
			humans.emplace_back(human);
		}
		else if (transform.name.find("Plate_Pavement") != std::string::npos) {
			float pos_x = transform.position.x;
			float pos_y = transform.position.y;
			int8_t x = static_cast<int8_t>(pos_x / TILE_SIZE);
			int8_t y = static_cast<int8_t>(pos_y / TILE_SIZE);
			Tile *new_tile = new Tile(&transform);
			board[std::make_pair(x+OFFSET, y+OFFSET)] = new_tile;
		} 
	}
	if (player == nullptr) throw std::runtime_error("Player not found.");
	for (int i = 0; i < zombie_count + human_count; i++) {
	  if (i < zombie_count && zombies[i] == nullptr)
	    throw std::runtime_error("Zombie" + std::to_string(i) + "not found.");
	  else if (i >= zombie_count && humans[i - zombie_count] == nullptr)
      	throw std::runtime_error("Human" + std::to_string(i - zombie_count) + "not found.");
	}
	active_tile = board[std::make_pair(active_tile_index.x, active_tile_index.y)];
	
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera->init_camera(player->position);
	//start music loop playing:
	// (note: position will be over-ridden in update())
	// zombie_1 = Sound::loop_3D(*zombie_sample_1, 1.0f, glm::vec3(0,0,0), 10.0f);
}

PlayMode::~PlayMode() {
	for (int i = 0; i < BOARD_WIDTH; i++) {
		for (int j = 0; j < BOARD_WIDTH; j++) {
			Tile* tile = board[std::make_pair(i, j)];
			delete tile;
		}
	}

	for (int i = 0; i < zombie_count; i++) {
		delete zombies[i];
	}

	for (int i = 0; i < human_count; i++) {
		delete humans[i];
	}
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			a.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			d.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			w.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			s.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			if (a.pressed) {
				a.released += 1;
				a.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			if (d.pressed) {
				d.released += 1;
				d.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			if (w.pressed) {
				w.released += 1;
				w.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			if (s.pressed) {
				s.released += 1;
				s.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			if (left.pressed) {
				left.released += 1;
				left.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			if (right.pressed) {
				right.released += 1;
				right.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			if (up.pressed) {
				up.released += 1;
				up.pressed = false;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			if (down.pressed) {
				down.released += 1;
				down.pressed = false;
			}
			return true;
		}
	} //----- trackball-style camera controls -----
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (evt.button.button == SDL_BUTTON_LEFT) {
			//when camera is upside-down at rotation start, azimuth rotation should be reversed:
			// (this ends up feeling more intuitive)
			camera->flip_x = (std::abs(camera->elevation) > 0.5f * 3.1415926f);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (evt.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			//figure out the motion (as a fraction of a normalized [-a,a]x[-1,1] window):
			glm::vec2 delta;
			delta.x = evt.motion.xrel / float(window_size.x) * 2.0f;
			delta.x *= float(window_size.y) / float(window_size.x);
			delta.y = evt.motion.yrel / float(window_size.y) * -2.0f;

			camera->azimuth -= 3.0f * delta.x * (camera->flip_x ? -1.0f : 1.0f);
			camera->elevation -= 3.0f * delta.y;

			camera->azimuth /= 2.0f * 3.1415926f;
			camera->azimuth -= std::round(camera->azimuth);
			camera->azimuth *= 2.0f * 3.1415926f;

			camera->elevation /= 2.0f * 3.1415926f;
			camera->elevation -= std::round(camera->elevation);
			camera->elevation *= 2.0f * 3.1415926f;
			return true;
		} 
	} else if (evt.type == SDL_MOUSEWHEEL) {
		//mouse wheel: dolly
		if (evt.type == SDL_MOUSEWHEEL) {
			camera->radius *= std::pow(0.5f, 0.1f * evt.wheel.y);
			if (camera->radius < 1e-1f) camera->radius = 1e-1f;
			if (camera->radius > 1e6f) camera->radius = 1e6f;
			return true;
		}
	}

	return false;
}

glm::ivec2 PlayMode::getActiveTileCoord() {
	return active_tile_index - glm::ivec2(OFFSET);
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);
	
	//move sound to follow leg tip position:
	// zombie_1->set_position(glm::vec3(0,0,0), 1.0f / 60.0f);
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 player_move = glm::vec2(0.0f);
		float theta = 0.f;
		if (a.released == 1) {
			player_move.x = -1.0f;
			a.released = 0;
			theta = LEFT - player_dir;
			player_dir = LEFT;
		} else if (d.released == 1) {
		  	player_move.x = 1.0f;
			d.released = 0;
			theta = RIGHT - player_dir;
			player_dir = RIGHT;
		} else if (s.released) {
		  	player_move.y =-1.0f;
			s.released = 0;
			theta = DOWN - player_dir;
			player_dir = DOWN;
		} else if (w.released) {
		  	player_move.y = 1.0f;
			w.released = 0;
			theta = UP - player_dir;
			player_dir = UP;
		}
		
    	player->rotation = player->rotation * glm::quat(cos(theta/2), 0, 0, sin(theta/2));

		// move player
		if ((camera->azimuth >= -M_PI_4) && (camera->azimuth < M_PI_4)) {
			player->position.y += player_move.y * PlayerSpeed * elapsed;
			player->position.x += player_move.x * PlayerSpeed * elapsed;
		} else if ((camera->azimuth >= M_PI_4) && (camera->azimuth < (M_PI - M_PI_4))) {
			player->position.y += player_move.x * PlayerSpeed * elapsed;
			player->position.x -= player_move.y * PlayerSpeed * elapsed;
		} else if ((camera->azimuth >= (-M_PI + M_PI_4)) && (camera->azimuth < -M_PI_4)) {
			player->position.y -= player_move.x * PlayerSpeed * elapsed;
			player->position.x += player_move.y * PlayerSpeed * elapsed;
		} else {
			player->position.y -= player_move.y * PlayerSpeed * elapsed;
			player->position.x -= player_move.x * PlayerSpeed * elapsed;
		}

		// move active tile
		glm::ivec2 tile_move = glm::ivec2(0);
		if (left.released == 1) {
			tile_move.x = -1;
			left.released = 0;
		}
		if (right.released == 1) {
			tile_move.x = 1;
			right.released = 0;
		}
		if (down.released == 1) {
			tile_move.y = -1;
			down.released = 0;
		}
		if (up.released == 1) {
			tile_move.y = 1;
			up.released = 0;
		}

		auto handleBoundry = [](glm::ivec2 &coord, int8_t max, int8_t min) {
			if (coord.x >= max) {
				coord.x = max-1;
			} else if (coord.x < min) {
				coord.x = min;
			}
			if (coord.y >= max) {
				coord.y = max-1;
			} else if (coord.y < min) {
				coord.y = min;
			}
		};
		
		active_tile->transform->position.z = 0.15; //original position read from blender
		
		if ((camera->azimuth >= -M_PI_4) && (camera->azimuth < M_PI_4)) {
			active_tile_index += tile_move;
		} else if ((camera->azimuth >= M_PI_4) && (camera->azimuth < (M_PI - M_PI_4))) {
			active_tile_index += glm::ivec2(-tile_move.y, tile_move.x);
		} else if ((camera->azimuth >= (-M_PI + M_PI_4)) && (camera->azimuth < -M_PI_4)) {
			active_tile_index += glm::ivec2(tile_move.y, -tile_move.x);
		} else {
			active_tile_index += glm::ivec2(-tile_move.x, -tile_move.y);
		}
		handleBoundry(active_tile_index, 7, 0);
		active_tile = board[std::make_pair(active_tile_index.x, active_tile_index.y)];
		active_tile->transform->position.z = -0.15;

		// move camera along with the player
		camera->target = player->position;

		if (space.pressed && active_tile != nullptr && active_tile->entity != nullptr) {
			if (active_tile->entity->character == Character::zombie && 
				!board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted) {
				points++;
				if (zombies_found < zombie_count) {
					glm::vec3 pos = glm::vec3(-0.7f, -0.7f, -1.3f);
					pos.x += (active_tile_index.x - floor(BOARD_WIDTH / 2)) * TILE_SIZE;
					pos.y += (active_tile_index.y - floor(BOARD_WIDTH / 2)) * TILE_SIZE;
					zombies[zombies_found]->transform->position = pos;
					zombies_found++;
				}
				board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted = true;
				} else if (active_tile->entity->character == human && 
						!board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted) {
					points--;
					if (humans_found < human_count) {
						glm::vec3 pos = glm::vec3(-0.7f, -0.7f, -1.3f);
						pos.x += (active_tile_index.x - floor(BOARD_WIDTH / 2)) * TILE_SIZE;
						pos.y += (active_tile_index.y - floor(BOARD_WIDTH / 2)) * TILE_SIZE;
							humans[humans_found]->transform->position = pos;
							humans_found++;
						board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted = true;
					}
				}
			space.pressed = false;
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	w.downs = 0;
	s.downs = 0;
	a.downs = 0;
	d.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->transform->rotation =
		glm::angleAxis(camera->azimuth, glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::angleAxis(0.5f * 3.1415926f + -camera->elevation, glm::vec3(1.0f, 0.0f, 0.0f))
	;
	camera->transform->position = camera->target + camera->radius * (camera->transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	camera->transform->scale = glm::vec3(1.0f);
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		constexpr float point_len = 0.35f;
		lines.draw_text("Mouse motion rotates camera; WASD moves player; Mouse wheel zooms in/out",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves player; Mouse wheel zooms in/out",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		glm::ivec2 active_tile_coord = getActiveTileCoord();
		lines.draw_text("Active tile: " + std::to_string(active_tile_coord.x) + ", " + std::to_string(active_tile_coord.y),
			glm::vec3(-aspect + 0.1f * H, 1.0 - H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Active tile: " + std::to_string(active_tile_coord.x) + ", " + std::to_string(active_tile_coord.y),
			glm::vec3(-aspect + 0.1f * H + ofs, 1.0 - H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text("Points: " + std::to_string(points),
			glm::vec3(aspect - point_len, 1.0 - H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text("Points: " + std::to_string(points),
			glm::vec3(aspect - point_len +ofs, 1.0 - H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

	}
	GL_ERRORS();
}

void PlayMode::initialize_board() {
	for (int i = 0; i < zombie_count + human_count; i++) {
		int r = rand() % BOARD_WIDTH;
		int c = rand() % BOARD_WIDTH;
		while (board[std::make_pair(r, c)]->entity != nullptr) {
			r = rand() % BOARD_WIDTH;
			c = rand() % BOARD_WIDTH;
		}
		if (i < zombie_count) {
			std::cout << "zombie " + std::to_string(i) + ": " + std::to_string(r) + ", " + std::to_string(c) + "\n";
			board[std::make_pair(r, c)]->entity = zombies[i];
			zombies[i]->tile = board[std::make_pair(r, c)];
		} else {
			std::cout << "human " + std::to_string(i) + ": " + std::to_string(r) + ", "+ std::to_string(c) + "\n";
			board[std::make_pair(r, c)]->entity = humans[i-zombie_count];
			humans[i-zombie_count]->tile = board[std::make_pair(r, c)];
		}
	}
}

// glm::vec3 PlayMode::get_leg_tip_position() {
// 	//the vertex position here was read from the model in blender:
// 	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
// }
