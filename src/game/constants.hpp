#pragma once

#include "SDL2/SDL.h"

namespace Color {
static constexpr auto white = SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF };
static constexpr auto red = SDL_Color{ 0xFF, 0x00, 0x00, 0xFF };
static constexpr auto blue = SDL_Color{ 0x00, 0xFF, 0x00, 0xFF };
static constexpr auto green = SDL_Color{ 0x00, 0x00, 0xFF, 0xFF };
static constexpr auto gray = SDL_Color{ 0x33, 0x33, 0x33, 0xFF };
static constexpr auto black = SDL_Color{ 0x00, 0x00, 0x00, 0xFF };
} // namespace color

namespace Constants {
static constexpr unsigned worldWidth = 1000; // of world units
static constexpr unsigned worldHeight = 750; // of world units

static constexpr unsigned maxTilesX = 10;
static constexpr unsigned maxTilesY = 10;

static constexpr unsigned tileWidth = worldWidth / maxTilesX; // of world units
static constexpr unsigned tileHeight =
  worldHeight / maxTilesY; // of world units

// the world space should be able to cover multiples of tiles
static_assert(worldWidth % tileWidth == 0);
static_assert(worldHeight % tileHeight == 0);

static constexpr float paddleWidth = worldWidth * 0.1;
static constexpr float paddleHeight = worldHeight * 0.01;

static constexpr float ballRadius = tileWidth * 0.15;
static constexpr float ballSpeed = 500;

} // namespace