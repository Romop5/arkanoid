#include "world.hpp"

#include <cstdlib>

namespace {
static const auto standardColors =
  std::vector<SDL_Color>{ Color::red, Color::blue, Color::green };

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
  initialize();
}

void
World::initialize()
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
  {
    ball.position = SDL_FPoint{ width / 2.0f, height - ball.radius * 2.0f };

    // initially: 1unit/second upward
    ball.speed = { -50, -(height / 3.0) };
  }

  // initialize paddle
  {
    paddle.body.w = 100;
    paddle.body.h = 10;
    paddle.body.x = (width / 2.0) - (paddle.body.w * 0.5);
    paddle.body.y = (height - 20) - (paddle.body.h * 0.5);
  }
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
  Paddle paddleBackup = paddle;

  // dry run: simulate movement and detect if any collision could happened on
  // the way
  updatePaddleDynamics(paddle, delta);
  updateBallDynamics(ball, delta);

  bool hasAnyCollision =
    detectBallCollisions(ball, false) || collidesBallWithWorldBoundaries(ball);

  // if ball has a potential collision, revert the state to initial and do
  // microstepping
  if (hasAnyCollision) {
    ball = ballBackup;
    paddle = paddleBackup;

    constexpr auto microstepsCount = 3;
    const auto microDelta = delta / microstepsCount;
    for (int i = 0; i < microstepsCount; i++) {
      updatePaddleDynamics(paddle, microDelta);
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

  // render paddle
  {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    SDL_FRect body = paddle.body;
    const auto rect = worldToViewCoordinates(app, body);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }
}

void
World::onKeyPressed(bool isKeyDown, SDL_Keysym key)
{
  SDL_Log("onKeyPressed: %d", isKeyDown);
  if (key.sym == SDLK_LEFT) {
    paddle.keys[ControllerKeys::move_left] = isKeyDown;
  }

  if (key.sym == SDLK_RIGHT) {
    paddle.keys[ControllerKeys::move_right] = isKeyDown;
  }
}

void
World::updatePaddleDynamics(Paddle& paddle, std::chrono::microseconds delta)
{
  const auto paddleSpeed = 1000;

  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);
  const auto movement = paddleSpeed * elapsedSeconds;

  const bool moveLeft = paddle.keys[ControllerKeys::move_left];
  const bool moveRight = paddle.keys[ControllerKeys::move_right];
  if (moveLeft)
    paddle.body.x -= movement;
  if (moveRight)
    paddle.body.x += movement;

  // keep within world
  paddle.body.x = std::clamp(paddle.body.x, 0.0f, width - paddle.body.w);
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

  std::pair<bool, bool> invertSpeed = { false, false };
  auto detectBallVsBodyCollision = [&](const SDL_FRect& body) -> bool {
    if (SDL_HasIntersectionF(&ballBody, &body)) {
      hasAnyCollision = true;

      invertSpeed = resolveBallSpeedCollisionAfter(body);
      return true;
    }
    return false;
  };

  for (const auto& tile : tileMap) {
    if (detectBallVsBodyCollision(tile.body) && reportCollisions) {
      events.push([=]() { onBallHitTile(tile.id); });
    }
  }

  // against paddle
  {
    const auto body = paddle.body;
    bool hasCollision = detectBallVsBodyCollision(body);
  }

  if (reportCollisions) {
    // adjust speed
    if (invertSpeed.first) {
      ball.speed.x *= -1.0f;
    }
    if (invertSpeed.second) {
      ball.speed.y *= -1.0f;
    }
  }

  return hasAnyCollision;
}

std::pair<bool, bool>
World::resolveBallSpeedCollisionAfter(SDL_FRect rect)
{
  const auto state = ball.getCollisionStateForGivenRect(rect);

  switch (state) {
    case Ball::CollisionState::no_collision:
      break;
    case Ball::CollisionState::from_left:
    case Ball::CollisionState::from_right: {
      return { true, false };
      break;
    }
    case Ball::CollisionState::from_above:
    case Ball::CollisionState::from_bottom: {
      return { false, true };
      break;
    }
    case Ball::CollisionState::left_top_corner:
    case Ball::CollisionState::right_top_corner:
    case Ball::CollisionState::left_bottom_corner:
    case Ball::CollisionState::right_bottom_corner:
    case Ball::CollisionState::full_inside: {
      return { true, true };
    }
  }

  return { false, false };
}

void
World::onBallHitTile(TileID id)
{
  SDL_Log("we hit it: %d", id);

  /*
  {
    // initialize ball
    ball.position = SDL_FPoint{ width / 2.0f, height - ball.radius * 2.0f };

    // initially: 1unit/second upward
    ball.speed = { 0, -(height / 3.0) };
  }*/

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