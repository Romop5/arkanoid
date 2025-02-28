#include "world.hpp"

#include <cstdlib>

namespace {
static const auto standardColors =
  std::vector<SDL_Color>{ Color::white, Color::red, Color::blue, Color::green };

auto
getRandomStandardColor() -> SDL_Color
{
  const auto i = std::rand() % standardColors.size();
  return standardColors[i];
};

} // namespace

World::World()
{
  // generate random tiles
  for (int x = 0; x < maxTilesX; x++) {
    for (int y = 0; y < maxTilesY - 3; y++) {

      bool skipTile = std::rand() % 2;
      if (skipTile) {
        continue;
      }

      SDL_Rect body;
      body.x = x * tileWidth;
      body.y = y * tileHeight;
      body.w = tileWidth;
      body.h = tileHeight;

      Tile tile;
      tile.body = body;
      tile.color = getRandomStandardColor();

      tileMap.push_back(tile);
    }
  }

  // initialize ball
  ball.position = SDL_FPoint{ width / 2.0f, height - ball.radius * 2.0f };

  // initially: 1unit/second upward
  ball.speed = { 0, -10 };
}

void
World::update(std::chrono::microseconds delta)
{
  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'1000.0);
  ball.position.x += elapsedSeconds * ball.speed.x;
  ball.position.y += elapsedSeconds * ball.speed.y;
}

void
World::render(Application& app)
{
  // render tiles
  for (const auto& tile : tileMap) {
    const auto& c = tile.color;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    const auto rect = worldToViewCoordinates(app, tile.body);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }

  // render ball
  {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    SDL_Rect ballBody;
    ballBody.x = ball.position.x - ball.radius;
    ballBody.y = ball.position.y - ball.radius;
    ballBody.w = 2.0 * ball.radius;
    ballBody.h = 2.0 * ball.radius;

    const auto rect = worldToViewCoordinates(app, ballBody);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }
}

SDL_Rect
World::worldToViewCoordinates(Application& app, SDL_Rect units)
{
  const auto widthRatio = static_cast<float>(app.surface->w) / width;
  const auto heightRatio = static_cast<float>(app.surface->h) / height;

  units.x *= widthRatio;
  units.w *= widthRatio;

  units.y *= heightRatio;
  units.h *= heightRatio;

  return units;
}
