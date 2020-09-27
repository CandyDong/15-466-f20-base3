#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

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
	} left, right, down, up, space;

	enum Character { none, zombie, human };

	struct Tile {
	    Scene::Transform *transform = nullptr;
	    Character character = none;
	};

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// scene models
	Scene::Transform *player = nullptr;
	std::vector<Scene::Transform *> tiles;
  std::vector<Tile> board;

  int board_width = 8;
  int zombie_count = 5;
  int human_count = 5;

	// mouse control
	bool left_mouse_down = false;
	bool right_mouse_down = false;

	// active 
	glm::uvec2 getActiveTileCoord();
	Tile *active_tile = nullptr;

	int points = 0;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > zombie_1;

	// outline thickness
	float outline = 3.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
