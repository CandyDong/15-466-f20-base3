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
#include <array>
#include <cassert>

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

Load< Sound::Sample > background_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Funshine.wav"));
});

Load< Sound::Sample > zombie_sample_1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("zombie_1.opus"));
});
Load< Sound::Sample > zombie_sample_2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("zombie_2.opus"));
});
Load< Sound::Sample > human_sample_1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("human_1.opus"));
});
Load< Sound::Sample > human_sample_2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("human_2.opus"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	for (auto &transform : scene.transforms) {
		if (transform.name == "Zombie") {
			player = new Entity(&transform, Character::zombie);
		}
		if (transform.name == "Zombie.001" || transform.name == "Zombie.002" ||
			transform.name == "Zombie.003" || transform.name == "Zombie.004" ||
			transform.name == "Zombie.005") {
			// the Tile pointer is to be populated in initialize_board
			Entity *zombie = new Entity(&transform, Character::zombie);
			zombies.emplace_back(zombie);
		}
		if (transform.name == "Human.001" || transform.name == "Human.002" ||
			transform.name == "Human.003" || transform.name == "Human.004" ||
			transform.name == "Human.005") {
			Entity *human = new Entity(&transform, Character::human);
			humans.emplace_back(human);
		}
		else if (transform.name.find("Plate_Pavement") != std::string::npos) {
			float pos_x = transform.position.x;
			float pos_y = transform.position.y;
			int8_t x = static_cast<int8_t>(pos_x / TILE_SIZE);
			int8_t y = static_cast<int8_t>(pos_y / TILE_SIZE);
			Tile *new_tile = new Tile(&transform, glm::ivec2(x+OFFSET, y+OFFSET));
			board[std::make_pair(x+OFFSET, y+OFFSET)] = new_tile;
		} 
	}
	if (player == nullptr) throw std::runtime_error("Player not found.");
	player->tile = board[std::make_pair(3, 3)]; // init player to be at the centre

	for (int i = 0; i < zombie_count + human_count; i++) {
	  if (i < zombie_count && zombies[i] == nullptr)
	    throw std::runtime_error("Zombie" + std::to_string(i) + "not found.");
	  else if (i >= zombie_count && humans[i - zombie_count] == nullptr)
      	throw std::runtime_error("Human" + std::to_string(i - zombie_count) + "not found.");
	}
	
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera->init_camera(player->transform->position);

	// start background music 
	Sound::loop(*background_sample, 0.1f, 0.0f);
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

	delete player;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (game_over) {
		return false;
	}

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
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
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

bool PlayMode::handleBoundry(glm::ivec2 &coord, int8_t max, int8_t min) {
	if (coord.x >= max) {
		coord.x = max-1;
		return true;
	} else if (coord.x < min) {
		coord.x = min;
		return true;
	}
	if (coord.y >= max) {
		coord.y = max-1;
		return true;
	} else if (coord.y < min) {
		coord.y = min;
		return true;
	}
	return false;
}

void PlayMode::update_sound() {
	glm::vec3 camera_position = camera->transform->position;
	// std::array<int8_t, 3> dirs = {1, 0, -1};
	for (int i = 0; i < BOARD_WIDTH; i++) {
		for (int j = 0; j < BOARD_WIDTH; j++) {
			Tile* tile = board[std::make_pair(i, j)];
			if (tile->entity == nullptr) {
				continue;
			}
			glm::ivec2 player_offset = glm::ivec2(i, j) - player->tile->index;
			glm::vec3 sound_position = camera_position + 
									glm::vec3(player_offset.x*3.0f, player_offset.y*3.0f, 
									player->transform->position.z);
			float volume = 1.0f;
			if ((std::abs(player_offset.x) >= 2) || (std::abs(player_offset.y) >= 2)) {
				volume = 0.0f;
			}
			if (tile->entity->character == Character::human) {
				if (tile->entity->sound == nullptr) {
					tile->entity->sound = Sound::loop_3D(*human_sample_2, volume, sound_position, 1.5f);
				} else {
					tile->entity->sound->set_position(sound_position);
					tile->entity->sound->set_volume(volume);
				}
				
			} else if (tile->entity->character == Character::zombie) {
				if (tile->entity->sound == nullptr) {
					tile->entity->sound = Sound::loop_3D(*zombie_sample_1, volume, sound_position, 1.5f);
				} else {
					tile->entity->sound->set_position(sound_position);
					tile->entity->sound->set_volume(volume);
				}
			}
		}
	}
}

void PlayMode::update_direction(Entity *character) {
	int rise = player->tile->index.y - character->tile->index.y;
	int run = player->tile->index.x - character->tile->index.x;
	float theta;
	if (run == 0 && rise > 0) theta = UP;
	else if (run == 0 && rise <= 0) theta = DOWN;
	else theta = atan(rise/run) + 3.1415926f/2;
	if (run < 0) theta += 3.1415926f;
	character->transform->rotation = glm::quat(cos(theta/2), 0, 0, sin(theta/2));
	character->transform->position = glm::vec3(character->tile->transform->position.x,
											character->tile->transform->position.y,
											ground_height);
}

void PlayMode::update(float elapsed) {
	if (game_over) {
		return;
	}

	{
		//combine inputs into a move:
		glm::ivec2 player_move = glm::vec2(0);
		float theta = 0.f;
		if (a.released == 1) {
			player_move.x = -1;
			a.released = 0;
			theta = LEFT - player_dir;
			player_dir = LEFT;
		} else if (d.released == 1) {
		  player_move.x = 1;
			d.released = 0;
			theta = RIGHT - player_dir;
			player_dir = RIGHT;
		} else if (s.released) {
		  player_move.y =-1;
			s.released = 0;
			theta = DOWN - player_dir;
			player_dir = DOWN;
		} else if (w.released) {
		  player_move.y = 1;
			w.released = 0;
			theta = UP - player_dir;
			player_dir = UP;
		}

		// rotate player according to direction headed
		player->transform->rotation = player->transform->rotation * glm::quat(cos(theta/2), 0, 0, sin(theta/2));

		glm::ivec2 player_tile_index = player->tile->index;
		// move player
		if ((camera->azimuth >= -M_PI_4) && (camera->azimuth < M_PI_4)) {
			player_tile_index += player_move;
		} else if ((camera->azimuth >= M_PI_4) && (camera->azimuth < (M_PI - M_PI_4))) {
			player_tile_index += glm::ivec2(-player_move.y, player_move.x);
		} else if ((camera->azimuth >= (-M_PI + M_PI_4)) && (camera->azimuth < -M_PI_4)) {
			player_tile_index += glm::ivec2(player_move.y, -player_move.x);
		} else {
			player_tile_index += glm::ivec2(-player_move.x, -player_move.y);
		}
		bool boundry_case = handleBoundry(player_tile_index, 7, 0);
		Tile* player_tile = board[std::make_pair(player_tile_index.x, player_tile_index.y)];
		player->tile = player_tile;
		update_sound(); //update sound based on player position

		if (pow(player_move.x, 2) + pow(player_move.y, 2) != 0) {
		  	jumping = true;
			// rotate characters according to player position
			for(int i = 0; i < human_count + zombie_count; i++) {
				if (i < human_count && humans[i]->tile->counted) {
					update_direction(humans[i]);
				} else if (i >= human_count) {
					int j = i - human_count;
					if (zombies[j]->tile->counted) {
						update_direction(zombies[j]);
					}
				}
			}
		}
		if ((!jumping) || (boundry_case)) {
			player->transform->position.x = player_tile->transform->position.x;
			player->transform->position.y = player_tile->transform->position.y;
			player->transform->position.z = ground_height;
		} else {
			float a = -1.0f;
			if (jump_count == 0) {
				prev_x = player->transform->position.x;
				prev_y = player->transform->position.y;
				delta = player_move;
			}
			if (delta.x == 0) {
				float b = prev_y + player->tile->transform->position.y;
				float c = -prev_y * player->tile->transform->position.y;
				float curY = prev_y + jump_count * delta.y * TILE_SIZE / jump_steps;
				float curZ = a * pow(curY, 2) + b * curY + c + ground_height;
				player->transform->position = glm::vec3(prev_x, curY, curZ);
			} else {
				float b = prev_x + player->tile->transform->position.x;
				float c = -prev_x  * player->tile->transform->position.x;
				float curX = prev_x + jump_count * delta.x * TILE_SIZE / jump_steps;
				float curZ = a * pow(curX, 2) + b * curX + c + ground_height;
				player->transform->position = glm::vec3(curX, prev_y, curZ);
			}
			jump_count++;
			if (jump_count == jump_steps) {
				jumping = false;
				jump_count = 0;
			}
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
		
		Tile* active_tile = board[std::make_pair(active_tile_index.x, active_tile_index.y)];
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
		// camera->target = player->transform->position;

		// if space pressed, check if the current active tile contains a human/zombie
		if (space.pressed) {
		  	assert(active_tile != nullptr);
			if (active_tile->entity == nullptr) {
				points--;
			} else {
				if (active_tile->entity->character == Character::zombie && 
					!board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted) {
					points++;
					if (zombies_found < zombie_count) {
						active_tile->entity->sound->set_volume(0.0f);
            			update_direction(active_tile->entity);
						zombies_found++;
						if (zombies_found == zombie_count) {
							game_over = true;
						}
					}
					board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted = true;
				} else if (active_tile->entity->character == Character::human &&
					!board[std::make_pair(active_tile_index.x, active_tile_index.y)]->counted) {
					points--;
					if (humans_found < human_count) {
						active_tile->entity->sound->set_volume(0.0f);
						update_direction(active_tile->entity);
						humans_found++;
					}
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
	if (game_over) {
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.8f)));
	} else {
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	}
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
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text("Points: " + std::to_string(points),
			glm::vec3(aspect - point_len +ofs, 1.0 - H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (game_over) {
			std::string text = "You've found all your zombies friends!! Your point is " + std::to_string(points);
			lines.draw_text(text,
				glm::vec3(-0.9f, -0.0f - H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text(text,
				glm::vec3(-0.9f + ofs, 0.0f - H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0x00, 0x00));
		}
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
			board[std::make_pair(r, c)]->entity = zombies[i];
			zombies[i]->tile = board[std::make_pair(r, c)];
		} else {
			board[std::make_pair(r, c)]->entity = humans[i-zombie_count];
			humans[i-zombie_count]->tile = board[std::make_pair(r, c)];
		}
	}
}