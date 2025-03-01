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

#include "application.hpp"
#include "ball.hpp"
#include <optional>

namespace Color {
static constexpr auto white = SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF };
static constexpr auto red = SDL_Color{ 0xFF, 0x00, 0x00, 0xFF };
static constexpr auto blue = SDL_Color{ 0x00, 0xFF, 0x00, 0xFF };
static constexpr auto green = SDL_Color{ 0x00, 0x00, 0xFF, 0xFF };
static constexpr auto gray = SDL_Color{ 0x33, 0x33, 0x33, 0xFF };
static constexpr auto black = SDL_Color{ 0x00, 0x00, 0x00, 0xFF };
} // namespace color

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

  void initialize();
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
  static constexpr unsigned width = 1000; // of world units
  static constexpr unsigned height = 750; // of world units

  static constexpr unsigned maxTilesX = 10;
  static constexpr unsigned maxTilesY = 10;

  static constexpr unsigned tileWidth = width / maxTilesX;   // of world units
  static constexpr unsigned tileHeight = height / maxTilesY; // of world units

  // the world space should be able to cover multiples of tiles
  static_assert(width % tileWidth == 0);
  static_assert(height % tileHeight == 0);

  std::vector<Tile> tileMap;

  std::optional<Ball> ball;

  Paddle paddle;

  //! FIFO of events
  std::queue<Event> events;
};