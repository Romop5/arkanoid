#pragma once

#include <SDL.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <chrono>
#include <queue>
#include <utility>
#include <vector>

#include "constants.hpp"
#include "application.hpp"
#include "ball.hpp"
#include <optional>


using TileID = unsigned;
struct Tile
{
  TileID id = -1;

  //! Position with width/height (in world units)
  SDL_FRect body = { 0, 0, 0, 0 };

  //! Color when drawing tile
  SDL_Color color = Color::white;

  //! How many count of collions remains before dying
  std::uint8_t lifes = 1;
};

enum ControllerKeys
{
  move_left = 0,
  move_right,
  size
};

struct Paddle
{
  SDL_FRect body;

  std::bitset<ControllerKeys::size> keys;
};

/**
 * @brief Logical definition of the world and its entities
 */
class World
{
public:
  using Event = std::function<void()>;

public:
  World();

  void initializeWorld();
  void initializeBall();

  void update(std::chrono::microseconds delta);
  void render(Application& app);

public:
  void onKeyPressed(bool isKeyDown, SDL_Keysym key);

protected:
  void updatePaddleDynamics(Paddle& paddle, std::chrono::microseconds delta);
  void updateBallDynamics(Ball& ball, std::chrono::microseconds delta);

  bool hasBallFallenDown(Ball& ball);
  bool collidesBallWithWorldBoundaries(Ball& ball);

  void correctBallAgainstWorldBoundaries(Ball& ball);
  bool detectBallCollisions(Ball& ball, bool reportCollisions);
  std::pair<bool, bool> resolveBallSpeedCollisionAfter(Ball& ball,
                                                       SDL_FRect rect);

protected:
  //! Event: ball hit tile
  void onBallHitTile(TileID id);

  //! Event: ball hit tile
  void onBallFallDown();

  SDL_Rect worldToViewCoordinates(Application&, SDL_FRect units);

private:
  std::vector<Tile> tileMap;

  std::optional<Ball> ball;

  Paddle paddle;

  //! FIFO of events
  std::queue<Event> events;
};