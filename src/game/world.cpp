#include "world.hpp"
#include "sdl_helper.hpp"

#include <cmath>
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
  result.x = static_cast<int>(std::roundf(frec.x));
  result.y = static_cast<int>(std::roundf(frec.y));
  result.w = static_cast<int>(std::roundf(frec.w));
  result.h = static_cast<int>(std::roundf(frec.h));

  return result;
}

} // namespace

World::World() {}

void
World::initializeWorld()
{
  // clear queue
  SDL_Log("Clearing event queue with n = %ull events", events.size());
  while (!events.empty()) {
    events.pop();
  }

  tileMap.clear();
  pickups.clear();

  // generate random tiles
  for (int x = 0; x < Constants::maxTilesX; x++) {
    for (int y = 0; y < Constants::maxTilesY - 3; y++) {

      bool skipTile = std::rand() % 2;
      if (skipTile) {
        continue;
      }

      SDL_FRect body;
      body.x = x * Constants::tileWidth;
      body.y = y * Constants::tileHeight;
      body.w = Constants::tileWidth;
      body.h = Constants::tileHeight;

      Tile tile;

      static unsigned id = 0;
      tile.id = id++;
      tile.body = body;
      tile.color = getRandomStandardColor();

      tileMap.push_back(tile);
    }
  }

  // Note: this must come before ball
  initializePaddle();
  initializeBall();

  gameStatus = GameStatus::running;

  gameState.remainingBalls = 3;

  setWorldSpeed(1.0);
}

void
World::initializeBall()
{
  ball = Ball();
  ball->radius = Constants::ballRadius;
  ball->position = SDL_FPoint{ paddle.body.x + paddle.body.w * 0.5f,
                               paddle.body.y - ball->radius - 1.0f };

  // initially: 1unit/second upward
  ball->speed = { ((std::rand() % 100) - 50.0f), -(Constants::ballSpeed) };
}

void
World::initializePaddle()
{
  paddle.body.w = Constants::paddleWidth;
  paddle.body.h = Constants::paddleHeight;
  paddle.body.x = (Constants::worldWidth / 2.0) - (paddle.body.w * 0.5);
  paddle.body.y = (Constants::worldHeight - Constants::paddleHeight * 1.2) -
                  (paddle.body.h * 0.5);
}

void
World::update(std::chrono::microseconds delta)
{
  const auto now = std::chrono::high_resolution_clock::now();
  using namespace std::chrono_literals;
  // slow down the game if FPS fall below 30 frames per second (33ms)
  // => prevent tunneling when updating movement and detecting collisions
  if (delta > 34ms) {
    delta = 34ms;
  }

  delta *= gameState.speed;

  // process events
  while (!events.empty()) {
    if (events.top().deadline < now) {
      SDL_Log("Popping event");
      const auto event = events.top();
      events.pop();

      // Fix: event could possibly destroy all events, including the one that is
      // being evaluated at the moments
      event.callback();

    } else {
      break;
    }
  }

  if (gameStatus != GameStatus::running) {
    return;
  }

  if (ball && hasBallFallenDown(*ball)) {
    events.push(Event([=]() { onBallFallDown(); }));
  }

  updatePickups(delta);

  Paddle paddleBackup = paddle;

  // dry run: simulate movement and detect if any collision could happened on
  // the way
  updatePaddleDynamics(paddle, delta);

  if (ball.has_value()) {
    Ball ballBackup = *ball;
    updateBallDynamics(*ball, delta);

    bool hasAnyCollision = detectBallCollisions(*ball, false) ||
                           collidesBallWithWorldBoundaries(*ball);

    // if ball has a potential collision, revert the state to initial and do
    // microstepping
    if (hasAnyCollision) {
      ball = ballBackup;
      paddle = paddleBackup;

      constexpr auto microstepsCount = 10;
      const auto microDelta = delta / microstepsCount;
      for (int i = 0; i < microstepsCount; i++) {
        updatePaddleDynamics(paddle, microDelta);
        updateBallDynamics(*ball, microDelta);
        correctBallAgainstWorldBoundaries(*ball);
        detectBallCollisions(*ball, true);
      }
    }
  }

  // update pickups and detect collisions
  // note: pickups are moving slow, we don't need to the maskarade above

  for (const auto& pickup : pickups) {
    if (SDL_HasIntersectionF(&pickup.body, &paddle.body)) {
    }
  }
}

void
World::render(Application& app)
{
  renderEntities(app);
  renderHUD(app);
}

void
World::renderEntities(Application& app)
{
  const auto appSize = app.getWindowSize();
  SDL_Rect viewport;
  viewport.x = 10;
  viewport.y = 50;
  viewport.w = appSize.x - 20;
  viewport.h = appSize.y - 50;

  SDL_RenderSetViewport(app.renderer.get(), &viewport);

  // render tiles
  const auto tileTexture = app.textures["tile"];
  for (const auto& entity : tileMap) {
    const auto& c = entity.color;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    const auto rect = worldToViewCoordinates(app, entity.body);

    SDL_RenderFillRect(app.renderer.get(), &rect);

    if (tileTexture) {
      SDL_SetTextureBlendMode(tileTexture, SDL_BLENDMODE_BLEND);
      SDL_RenderCopy(app.renderer.get(), tileTexture, NULL, &rect);
      SDL_SetTextureBlendMode(tileTexture, SDL_BLENDMODE_NONE);
    }
  }

  // render ball
  if (ball) {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    SDL_FRect ballBody = ball->getBoundingRect();
    const auto rect = worldToViewCoordinates(app, ballBody);

    const auto ballTexture = app.textures["ball"];
    if (ballTexture) {
      SDL_SetTextureBlendMode(ballTexture, SDL_BLENDMODE_BLEND);
      SDL_RenderCopy(app.renderer.get(), ballTexture, NULL, &rect);
      SDL_SetTextureBlendMode(ballTexture, SDL_BLENDMODE_NONE);
    } else {
      SDL_RenderFillRect(app.renderer.get(), &rect);
    }
  }

  // render paddle
  {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    SDL_FRect body = paddle.body;
    const auto rect = worldToViewCoordinates(app, body);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }

  // render pickups
  for (const auto& entity : pickups) {
    const auto& c = entity.color;
    SDL_SetRenderDrawColor(app.renderer.get(), c.r, c.g, c.b, c.a);

    const auto rect = worldToViewCoordinates(app, entity.body);

    SDL_RenderFillRect(app.renderer.get(), &rect);
  }

  SDL_RenderSetViewport(app.renderer.get(), nullptr);
}

void
World::renderHUD(Application& app)
{
  if (gameStatus == GameStatus::running) {

    // render lives
    {
      const auto blackText = app.getCachedTextureForText(
        std::format("Lives: {}", gameState.remainingBalls));
      const auto textSize = sdl_helper::getsize(blackText);

      const auto ws = app.getWindowSize();
      SDL_Rect rect;
      rect.x = 5;
      rect.y = 5;
      rect.w = textSize.x;
      rect.h = textSize.y;

      SDL_RenderCopy(app.renderer.get(), blackText, NULL, &rect);
    }

    // render score
    {
      const auto blackText =
        app.getCachedTextureForText(std::format("Score: {}", gameState.score));
      const auto textSize = sdl_helper::getsize(blackText);

      const auto ws = app.getWindowSize();
      SDL_Rect rect;
      rect.x = ws.x - 5 - textSize.x;
      rect.y = 5;
      rect.w = textSize.x;
      rect.h = textSize.y;

      SDL_RenderCopy(app.renderer.get(), blackText, NULL, &rect);
    }
  }

  if (gameStatus != GameStatus::running) {
    std::string textureName;

    if (gameStatus == GameStatus::initial_screen) {
      textureName = "arkanoid";
    }

    if (gameStatus == GameStatus::game_over) {
      textureName = "game_over";
    }
    if (gameStatus == GameStatus::you_won) {
      textureName = "you_won";
    }
    const auto overlayTexture = app.textures[textureName];
    SDL_SetTextureBlendMode(overlayTexture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(app.renderer.get(), overlayTexture, NULL, NULL);
    SDL_SetTextureBlendMode(overlayTexture, SDL_BLENDMODE_NONE);
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

  if (key.sym == SDLK_SPACE && isKeyDown) {
    onReleaseBall();
  }

  if (key.sym == SDLK_r && isKeyDown) {
    onRestart();
  }

  if (key.sym == SDLK_RETURN && isKeyDown) {
    if (gameStatus == GameStatus::initial_screen) {
      onRestart();
    }
  }
}

void
World::updatePickups(std::chrono::microseconds delta)
{
  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);

  for (auto it = pickups.begin(); it != pickups.end(); it++) {
    bool shouldErase = false;
    auto& pickup = *it;
    const auto initialBody = pickup.body;

    // update dynamics
    pickup.body.y += Constants::pickupFallSpeed * elapsedSeconds;

    // detect falling out of world
    if (pickup.body.y > Constants::worldHeight - pickup.body.h * 0.5) {
      events.push(Event([=]() { onPickupFallDown(pickup.id); }));
      continue;
    }

    // detect colliding paddle
    SDL_FRect pickupConvexHull = initialBody;
    pickupConvexHull.h = pickup.body.y - initialBody.y + initialBody.h;

    if (SDL_HasIntersectionF(&pickupConvexHull, &paddle.body)) {
      events.push(Event([=]() { onPickupPicked(pickup.id); }));
    }
  }
}

void
World::updatePaddleDynamics(Paddle& paddle, std::chrono::microseconds delta)
{
  const auto speed = paddle.getCurrentSpeed();

  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);
  const auto movement = speed * elapsedSeconds;
  paddle.body.x += movement;

  // keep within world
  paddle.body.x =
    std::clamp(paddle.body.x, 0.0f, Constants::worldWidth - paddle.body.w);
}

void
World::updateBallDynamics(Ball& ball, std::chrono::microseconds delta)
{
  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);

  ball.position.x += elapsedSeconds * ball.speed.x;
  ball.position.y += elapsedSeconds * ball.speed.y;
}

bool
World::hasBallFallenDown(Ball& ball)
{
  const bool isBelowWorld =
    (ball.position.y + ball.radius > (Constants::worldHeight - 10));
  return isBelowWorld;
}

bool
World::collidesBallWithWorldBoundaries(Ball& ball)
{
  const bool isAboveWorld = (ball.position.y - ball.radius < 0);
  const bool isBelowWorld =
    (ball.position.y + ball.radius > Constants::worldHeight);
  const bool isLeftWorld = (ball.position.x - ball.radius < 0);
  const bool isRightWorld =
    (ball.position.x + ball.radius > Constants::worldWidth);

  return isAboveWorld || isBelowWorld || isLeftWorld || isRightWorld;
}

void
World::correctBallAgainstWorldBoundaries(Ball& ball)
{
  bool isAboveWorld = (ball.position.y - ball.radius < 0);
  bool isBelowWorld = (ball.position.y + ball.radius > Constants::worldHeight);
  bool isLeftWorld = (ball.position.x - ball.radius < 0);
  bool isRightWorld = (ball.position.x + ball.radius > Constants::worldWidth);

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
    ball.position.y = Constants::worldHeight - ball.radius;
  }

  if (isLeftWorld) {
    ball.position.x = ball.radius;
  }

  if (isRightWorld) {
    ball.position.x = Constants::worldWidth - ball.radius;
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

      invertSpeed = resolveBallSpeedCollisionAfter(ball, body);
      return true;
    }
    return false;
  };

  for (const auto& tile : tileMap) {
    if (detectBallVsBodyCollision(tile.body) && reportCollisions) {
      events.push(Event([=]() { onBallHitTile(tile.id); }));
    }
  }

  // against paddle

  bool hasPaddleCollision = false;
  {
    const auto body = paddle.body;
    hasPaddleCollision = detectBallVsBodyCollision(body);
  }

  if (reportCollisions) {
    // adjust speed
    if (invertSpeed.first) {
      ball.speed.x *= -1.0f;
    }
    if (invertSpeed.second) {
      ball.speed.y *= -1.0f;
    }

    if (hasPaddleCollision) {
      const auto speed = paddle.getCurrentSpeed();
      ball.speed.x += speed * 0.3;
    }
  }

  return hasAnyCollision;
}

std::pair<bool, bool>
World::resolveBallSpeedCollisionAfter(Ball& ball, SDL_FRect rect)
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
World::spawnRandomPickup(SDL_Point position, SDL_Color color)
{
  static unsigned id = 0;

  Pickup pickup;
  pickup.id = id++;

  // Choose random type
  pickup.type = static_cast<Pickup::Type>(std::rand() % Pickup::Type::size);

  pickup.body.w = Constants::tileWidth * 0.5;
  pickup.body.h = Constants::tileHeight * 0.5;
  pickup.body.x = position.x + pickup.body.w / 2.0;
  pickup.body.y = position.y + pickup.body.w / 2.0;

  pickup.color = color;
  pickups.push_back(pickup);
}

void
World::setWorldSpeed(float ratio)
{
  gameState.speed = ratio;
}

void
World::setBallSize(float ratio)
{
  if (ball) {
    ball->radius *= ratio;
  }
}

void
World::onRestart()
{
  SDL_Log("onRestart");
  initializeWorld();
}

void
World::onLevelFinished()
{
  SDL_Log("onLevelFinished");
  gameStatus = GameStatus::you_won;
  events.push(Event(std::chrono::seconds(10), [=]() { onRestart(); }));
}

void
World::onGameOver()
{
  SDL_Log("onGameOver");
  gameStatus = GameStatus::game_over;
  events.push(Event(std::chrono::seconds(10), [=]() { onRestart(); }));
}

void
World::onReleaseBall()
{
  SDL_Log("onReleaseBall");
  if (!ball.has_value()) {
    initializeBall();
  }
}

void
World::onBallHitTile(EntityID id)
{
  SDL_Log("onBallHitTile: %d", id);

  auto it = std::find_if(tileMap.begin(), tileMap.end(), [=](const Tile& tile) {
    return tile.id == id;
  });

  bool willBeDestroyed = false;
  // delete tile if is dead
  if (it != tileMap.end()) {
    it->lifes--;

    if (it->lifes == 0) {
      willBeDestroyed = true;

      gameState.score += Constants::rewardTileDestroyed;
    }
  }

  // when tile was destroyed and with some lower chance, generate special pickup
  if (willBeDestroyed && std::rand() % 5 == 0) {
    SDL_Point spawnPoint = { it->body.x, it->body.y };
    spawnRandomPickup(spawnPoint, it->color);
  }
  if (willBeDestroyed) {
    // destroy it
    tileMap.erase(it);
  }

  if (tileMap.empty()) {
    onLevelFinished();
  }
}

void
World::onBallFallDown()
{
  SDL_Log("onBallFallDown");
  ball.reset();

  gameState.score -= Constants::penaltyLostBall;
  if (gameState.remainingBalls == 0) {
    onGameOver();
  } else {
    gameState.remainingBalls--;
  }
}

void
World::onPickupPicked(EntityID pickupId)
{
  SDL_Log("onPickupPicked: %d", pickupId);
  auto it =
    std::find_if(pickups.begin(), pickups.end(), [=](const Pickup& entity) {
      return entity.id == pickupId;
    });

  // process events
  if (it != pickups.end()) {

    gameState.score += Constants::rewardPickupPicked;

    const auto& pickup = *it;
    switch (pickup.type) {
      case Pickup::Type::speedup: {
        setWorldSpeed(2.0);
        events.push(
          Event(std::chrono::seconds(10), [=]() { setWorldSpeed(1.0); }));
        break;
      }

      case Pickup::Type::slowdown: {
        setWorldSpeed(1.0);
        events.push(
          Event(std::chrono::seconds(10), [=]() { setWorldSpeed(1.0); }));
        break;
      }

      case Pickup::Type::change_ball_size: {

        constexpr float minimalRadius = 5.0;
        if (ball && ball->radius < minimalRadius)
          break;

        setBallSize(0.5);
        events.push(
          Event(std::chrono::seconds(10), [=]() { setBallSize(2.0); }));
        break;
      }

      case Pickup::Type::change_paddle_size: {
        // until restart
        paddle.body.w *= 2;
        paddle.body.w =
          std::clamp(paddle.body.w, 0.0f, Constants::worldWidth * 0.99f);

        if (Constants::worldWidth < (paddle.body.x + paddle.body.w)) {
          paddle.body.x = std::clamp(
            paddle.body.x, 0.0f, Constants::worldWidth - paddle.body.w);
        }
        break;
      }
    }
  }

  // delete it
  if (it != pickups.end()) {
    pickups.erase(it);
  }
}

void
World::onPickupFallDown(EntityID pickupId)
{
  SDL_Log("onPickupFallDown: %d", pickupId);
  auto it =
    std::find_if(pickups.begin(), pickups.end(), [=](const Pickup& entity) {
      return entity.id == pickupId;
    });
  // delete it
  if (it != pickups.end()) {
    pickups.erase(it);
  }
}

SDL_Rect
World::worldToViewCoordinates(Application& app, SDL_FRect units)
{
  SDL_Rect viewport;
  SDL_RenderGetViewport(app.renderer.get(), &viewport);

  const auto widthRatio =
    static_cast<float>(viewport.w) / Constants::worldWidth;
  const auto heightRatio =
    static_cast<float>(viewport.h) / Constants::worldHeight;

  units.x *= widthRatio;
  units.w *= widthRatio;

  units.y *= heightRatio;
  units.h *= heightRatio;

  return frecTorec(units);
}

float
Paddle::getCurrentSpeed() const
{
  const auto movement = Constants::paddleSpeed;

  const bool moveLeft = keys[ControllerKeys::move_left];
  const bool moveRight = keys[ControllerKeys::move_right];

  float speed = 0.0f;
  if (moveLeft)
    speed -= movement;
  if (moveRight)
    speed += movement;

  return speed;
}
