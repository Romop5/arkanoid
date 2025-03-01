#pragma once

#include <SDL.h>
#include <chrono>
#include <vector>
#include <queue>

#include "application.hpp"

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

struct Ball
{
  SDL_FPoint position = { 0, 0 };
  SDL_FPoint speed = { 0, 0 }; // units/second
  float radius = 50.0;

  SDL_FRect getBoundingRect()
  {
    SDL_FRect ballBody;
    ballBody.x = position.x - radius;
    ballBody.y = position.y - radius;
    ballBody.w = 2.0 * radius;
    ballBody.h = 2.0 * radius;
    return ballBody;
  }
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

  void update(std::chrono::microseconds delta);
  void render(Application& app);

protected:
  void updateBallDynamics(Ball& ball, std::chrono::microseconds delta);
  bool collidesBallWithWorldBoundaries(Ball& ball);
  void correctBallAgainstWorldBoundaries(Ball& ball);
  bool detectBallCollisions(Ball& ball, bool reportCollisions);

protected:
  //! Event: ball hit tile
  void onBallHitTile(TileID id);

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

  Ball ball;

  //! FIFO of events
  std::queue<Event> events;
};