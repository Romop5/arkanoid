#pragma once

#include "SDL2/SDL.h"

namespace Color {
static constexpr auto white = SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF };
static constexpr auto dark_white = SDL_Color{ 0xCC, 0xCC, 0xCC, 0xFF };
static constexpr auto red = SDL_Color{ 0xFF, 0x00, 0x00, 0xFF };
static constexpr auto blue = SDL_Color{ 0x00, 0xFF, 0x00, 0xFF };
static constexpr auto green = SDL_Color{ 0x00, 0x00, 0xFF, 0xFF };
static constexpr auto gray = SDL_Color{ 0x33, 0x33, 0x33, 0xFF };
static constexpr auto black = SDL_Color{ 0x00, 0x00, 0x00, 0xFF };
} // namespace color

namespace Constants {
// Screen dimension constants
constexpr int screenWidth = 640;  // px
constexpr int screenHeight = 480; // px

static constexpr unsigned worldRenderingHorizontalMargin = 10; // px
static constexpr unsigned worldRenderingTopMargin = 50;        // px

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

static constexpr float paddleWidth = worldWidth * 0.2;
static constexpr float paddleHeight = worldHeight * 0.01;
static constexpr float ballRadius = tileWidth * 0.15;

static constexpr float paddleSpeed = 1000;    // world units * s^-1
static constexpr float ballSpeed = 500;       // world units * s^-1
static constexpr float pickupFallSpeed = 300; // world units * s^-1

static constexpr unsigned penaltyLostBall = 100;    // score points
static constexpr unsigned rewardTileDestroyed = 10; // score points
static constexpr unsigned rewardPickupPicked = 1;   // score points

} // namespace