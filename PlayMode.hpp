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

	bool game_over = false;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t released = 0;
	} w, s, a, d, left, right, down, up, space;

	enum Character { none, zombie, human };

	struct Tile; // Forward-declare one of the structs to compile
	// entity must have a nonnull character value
	struct Entity {
		Entity(Scene::Transform *trans, Character c) {
			transform = trans;
			character = c;
			if (c == Character::none) {
				throw std::runtime_error("Entity must have a non-null character value.");
			}
		}
		Scene::Transform *transform = nullptr;
		Character character = none;
		Tile* tile = nullptr;
		std::shared_ptr< Sound::PlayingSample > sound = nullptr;
		bool rotated = false;
	};

	virtual void update_direction(std::vector<Entity *> chars, int index);

	struct Tile {
		Tile(Scene::Transform *t, glm::ivec2 ind) {
			transform = t;
			index = ind;
		} 
		Scene::Transform *transform = nullptr;
		Entity* entity = nullptr;
		bool counted = false;
		glm::ivec2 index;
	};

	//direction that the character is facing
	float DOWN = 0.f;
	float RIGHT = 3.1415926f / 2;
	float UP = 3.1415926f;
	float LEFT = 1.5f * 3.1415926f;
	float player_dir = DOWN;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// scene models
	Entity *player = nullptr;
	std::vector<Entity *> humans;
	std::vector<Entity *> zombies;

	// maps two coords to a tile
	std::map<std::pair<int8_t, int8_t>, Tile *> board; 

	uint8_t TILE_SIZE = 3;
	uint8_t BOARD_WIDTH = 7;
	uint8_t OFFSET = TILE_SIZE;
	
	int zombie_count = 4;
	int human_count = 5;
	int zombies_found = 0;
	int humans_found = 0;

	bool jumping = false;
	int jump_steps = 10;
	int jump_count = 0;
	float prev_x, prev_y;
	glm::vec2 delta = glm::vec2(0);

	float ground_height = -1.4f;

	// active 
	glm::ivec2 getActiveTileCoord();
	glm::ivec2 active_tile_index = glm::ivec2(3, 3);
	void handleBoundry(glm::ivec2 &coord, int8_t max, int8_t min);

	// update sound with player movement
	void updateSound();

	int points = 0;

	// outline thickness
	float outline = 3.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
