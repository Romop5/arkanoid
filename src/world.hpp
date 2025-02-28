#pragma once

#include <SDL.h>
#include <vector>
#include <chrono>

#include "application.hpp"

namespace Color {
static constexpr auto white = SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF };
static constexpr auto red = SDL_Color{ 0xFF, 0x00, 0x00, 0xFF };
static constexpr auto blue = SDL_Color{ 0x00, 0xFF, 0x00, 0xFF };
static constexpr auto green = SDL_Color{ 0x00, 0x00, 0xFF, 0xFF };
static constexpr auto gray = SDL_Color{ 0x33, 0x33, 0x33, 0xFF };
static constexpr auto black = SDL_Color{ 0x00, 0x00, 0x00, 0xFF };
} // namespace color

struct Tile
{
  //! Position with width/height (in world units)
  SDL_Rect body = { 0, 0, 0, 0 };

  //! Color when drawing tile
  SDL_Color color = Color::white;

  //! How many count of collions remains before dying
  std::uint8_t lifes = 1;
};

/**
 * @brief Logical definition of the world and its entities
 */
class World
{
public:
  World();

  void update(std::chrono::microseconds delta);
  void render(Application& app);

protected:
  SDL_Rect worldToViewCoordinates(Application&, SDL_Rect units);

private:
  static constexpr unsigned width = 100;  // of world units
  static constexpr unsigned height = 300; // of world units

  static constexpr unsigned maxTilesX = 10;
  static constexpr unsigned maxTilesY = 10;

  static constexpr unsigned tileWidth = width / maxTilesX; // of world units
  static constexpr unsigned tileHeight = height / maxTilesY; // of world units

  // the world space should be able to cover multiples of tiles
  static_assert(width % tileWidth == 0);
  static_assert(height % tileHeight == 0);

  std::vector<Tile> tileMap;

  SDL_FPoint ball;
  float ballRadius = 5.0;
};