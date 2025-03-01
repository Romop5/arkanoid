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

auto
frecTorec(SDL_FRect frec) -> SDL_Rect
{
  SDL_Rect result;
  result.x = static_cast<int>(frec.x);
  result.y = static_cast<int>(frec.y);
  result.w = static_cast<int>(frec.w);
  result.h = static_cast<int>(frec.h);

  return result;
}

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

      SDL_FRect body;
      body.x = x * tileWidth;
      body.y = y * tileHeight;
      body.w = tileWidth;
      body.h = tileHeight;

      Tile tile;

      static unsigned id = 0;
      tile.id = id++;
      tile.body = body;
      tile.color = getRandomStandardColor();

      tileMap.push_back(tile);
    }
  }

  // initialize ball
  ball.position = SDL_FPoint{ width / 2.0f, height - ball.radius * 2.0f };

  // initially: 1unit/second upward
  ball.speed = { -50, -(height / 3.0) };
}

void
World::update(std::chrono::microseconds delta)
{
  using namespace std::chrono_literals;
  // slow down the game if FPS fall below 30 frames per second (33ms)
  // => prevent tunneling when updating movement and detecting collisions
  if (delta > 34ms) {
    delta = 34ms;
  }

  // process events
  while (!events.empty()) {
    events.front()();
    events.pop();
  }

  Ball ballBackup = ball;
  
  // dry run: simulate movement and detect if any collision could happened on the way
  updateBallDynamics(ball, delta);
  bool hasAnyCollision = detectBallCollisions(ball, false);

  // if ball has a potential collision, revert the state to initial and do microstepping
  if (hasAnyCollision) {
    ball = ballBackup;

    constexpr auto microstepsCount = 3;
    const auto microDelta = delta / microstepsCount;
    for (int i = 0; i < microstepsCount; i++) {
      updateBallDynamics(ball, microDelta);
      correctBallAgainstWorldBoundaries(ball);
      detectBallCollisions(ball, true);
    }
  }
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

    SDL_FRect ballBody = ball.getBoundingRect();
    const auto rect = worldToViewCoordinates(app, ballBody);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }
}

void
World::updateBallDynamics(Ball& ball, std::chrono::microseconds delta)
{
  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);

  ball.position.x += elapsedSeconds * ball.speed.x;
  ball.position.y += elapsedSeconds * ball.speed.y;
}

bool
World::collidesBallWithWorldBoundaries(Ball& ball)
{
  bool isAboveWorld = (ball.position.y - ball.radius < 0);
  bool isBelowWorld = (ball.position.y + ball.radius > height);
  bool isLeftWorld = (ball.position.x - ball.radius < 0);
  bool isRightWorld = (ball.position.x + ball.radius > width);

  return isAboveWorld || isBelowWorld || isLeftWorld || isRightWorld;
}

void
World::correctBallAgainstWorldBoundaries(Ball& ball)
{
  bool isAboveWorld = (ball.position.y - ball.radius < 0);
  bool isBelowWorld = (ball.position.y + ball.radius > height);
  bool isLeftWorld = (ball.position.x - ball.radius < 0);
  bool isRightWorld = (ball.position.x + ball.radius > width);

  if ((isAboveWorld && ball.speed.y < 0) ||
      (isBelowWorld && ball.speed.y > 0)) {
    ball.speed.y *= -1;
  }

  if (isLeftWorld || isRightWorld) {
    ball.speed.x *= -1;
  }

  if (isAboveWorld) {
    ball.position.y = ball.radius;
  }

  if (isBelowWorld) {
    ball.position.y = height - ball.radius;
  }

  if (isLeftWorld) {
    ball.position.x = ball.radius;
  }

  if (isRightWorld) {
    ball.position.x = width - ball.radius;
  }
}

bool
World::detectBallCollisions(Ball& ball, bool reportCollisions)
{
  bool hasAnyCollision = false;
  const auto ballBody = ball.getBoundingRect();
  for (const auto& tile : tileMap) {
    if (SDL_HasIntersectionF(&ballBody, &tile.body)) {
      if (!reportCollisions) {
        return true;
      }
      hasAnyCollision = true;
      events.push([=]() { onBallHitTile(tile.id); });

      // correct speed/position?


    }
  }

  return hasAnyCollision;
}

void
World::onBallHitTile(TileID id)
{
  SDL_Log("we hit it: %d", id);

  // initialize ball
  ball.position = SDL_FPoint{ width / 2.0f, height - ball.radius * 2.0f };

  // initially: 1unit/second upward
  ball.speed = { 0, -(height / 3.0) };

  auto it = std::find_if(tileMap.begin(), tileMap.end(), [=](const Tile& tile) {
    return tile.id == id;
  });

  // delete tile if is dead
  if (it != tileMap.end()) {
    it->lifes--;

    if (it->lifes == 0) {
      tileMap.erase(it);
    }
  }
}

SDL_Rect
World::worldToViewCoordinates(Application& app, SDL_FRect units)
{
  const auto widthRatio = static_cast<float>(app.surface->w) / width;
  const auto heightRatio = static_cast<float>(app.surface->h) / height;

  units.x *= widthRatio;
  units.w *= widthRatio;

  units.y *= heightRatio;
  units.h *= heightRatio;

  return frecTorec(units);
}