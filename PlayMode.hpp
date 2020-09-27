#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t released = 0;
	} w, s, a, d, left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// scene models
	Scene::Transform *player = nullptr;
	std::array<std::array<Scene::Transform *, 7>, 7> tiles; 
	uint8_t tile_size = 3;
	uint8_t plane_size = 7;
	uint8_t offset = tile_size;

	// active 
	glm::ivec2 getActiveTileCoord();
	glm::ivec2 active_tile_index = glm::ivec2(3, 3);
	Scene::Transform *active_tile = nullptr;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > zombie_1;

	// outline thickness
	float outline = 3.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
