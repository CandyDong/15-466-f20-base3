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

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("test.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("test.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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
		if (transform.name == "Cube") player = &transform;
		else if (transform.name == "Plane") plane = &transform;
	}
	if (player == nullptr) throw std::runtime_error("Player not found.");
	if (plane == nullptr) throw std::runtime_error("Plane not found.");

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	camera->init_camera(player->position);

	//start music loop playing:
	// (note: position will be over-ridden in update())
	zombie_1 = Sound::loop_3D(*zombie_sample_1, 1.0f, glm::vec3(0,0,0), 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
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
	}//----- trackball-style camera controls -----
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

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// wobble += elapsed / 10.0f;
	// wobble -= std::floor(wobble);
	
	//move sound to follow leg tip position:
	zombie_1->set_position(glm::vec3(0,0,0), 1.0f / 60.0f);

	//move camera:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;
		
		// move player
		if ((camera->azimuth >= -M_PI_4) && (camera->azimuth < M_PI_4)) {
			player->position.y += move.y * PlayerSpeed * elapsed;
			player->position.x += move.x * PlayerSpeed * elapsed;
		} else if ((camera->azimuth >= M_PI_4) && (camera->azimuth < (M_PI - M_PI_4))) {
			player->position.y += move.x * PlayerSpeed * elapsed;
			player->position.x -= move.y * PlayerSpeed * elapsed;
		} else if ((camera->azimuth >= (-M_PI + M_PI_4)) && (camera->azimuth < -M_PI_4)) {
			player->position.y -= move.x * PlayerSpeed * elapsed;
			player->position.x += move.y * PlayerSpeed * elapsed;
		} else {
			player->position.y -= move.y * PlayerSpeed * elapsed;
			player->position.x -= move.x * PlayerSpeed * elapsed;
		}
		// move camera along with the player
		camera->target = player->position;
		
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
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

// glm::vec3 PlayMode::get_leg_tip_position() {
// 	//the vertex position here was read from the model in blender:
// 	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
// }
