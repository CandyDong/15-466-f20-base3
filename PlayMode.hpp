#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual void initialize_board() override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t released = 0;
	} w, s, a, d, left, right, down, up, space;

	enum Character { none, zombie, human };

	struct Tile {
		Tile(Scene::Transform *t, Character c) {
			transform = t;
			character = c;
			counted = false;
		} 
	    Scene::Transform *transform = nullptr;
	    Character character = none;
	    bool counted = false;
	};

	//direction that the character is facing
	float DOWN = 0.f;
	float RIGHT = 3.1415926f / 2;
	float UP = 3.1415926f;
	float LEFT = 1.5f * 3.1415926f;
	float player_dir = 0.f;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// scene models
	Scene::Transform *player = nullptr;
	std::vector<Scene::Transform *> humans;
	std::vector<Scene::Transform *> zombies;

	// maps two coords to a tile
	std::map<std::pair<int8_t, int8_t>, Tile *> tileCoordMap; 
	std::vector<Tile *> board;

	uint8_t TILE_SIZE = 3;
	uint8_t BOARD_WIDTH = 7;
	uint8_t OFFSET = TILE_SIZE;
	
	int zombie_count = 5;
	int human_count = 5;
	int zombies_found = 0;
	int humans_found = 0;

	// active 
	glm::ivec2 getActiveTileCoord();
	glm::ivec2 active_tile_index = glm::ivec2(3, 3);
	Tile *active_tile = nullptr;

	int points = 0;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > zombie_1;

	// outline thickness
	float outline = 3.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
