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
  SDL_Log("Clearing event queue with n = %ull events", m_events.size());
  while (!m_events.empty()) {
    m_events.pop();
  }

  m_tileMap.clear();
  m_pickups.clear();

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

      m_tileMap.push_back(tile);
    }
  }

  // Note: this must come before ball
  initializePaddle();
  initializeBall();

  m_gameStatus = GameStatus::running;

  m_gameState.remainingBalls = 3;

  setWorldSpeed(1.0);
}

void
World::initializeBall()
{
  m_ball = Ball();
  m_ball->radius = Constants::ballRadius;
  m_ball->position = SDL_FPoint{ m_paddle.body.x + m_paddle.body.w * 0.5f,
                                 m_paddle.body.y - m_ball->radius - 1.0f };

  // initially: 1unit/second upward
  m_ball->speed = { ((std::rand() % 100) - 50.0f), -(Constants::ballSpeed) };
}

void
World::initializePaddle()
{
  m_paddle.body.w = Constants::paddleWidth;
  m_paddle.body.h = Constants::paddleHeight;
  m_paddle.body.x = (Constants::worldWidth / 2.0) - (m_paddle.body.w * 0.5);
  m_paddle.body.y = (Constants::worldHeight - Constants::paddleHeight * 1.2) -
                    (m_paddle.body.h * 0.5);
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

  delta *= m_gameState.speed;

  // process events
  while (!m_events.empty()) {
    if (m_events.top().getDeadline() < now) {
      SDL_Log("Popping event");
      const auto event = m_events.top();
      m_events.pop();

      // Fix: event could possibly destroy all events, including the one that is
      // being evaluated at the moments
      event.getCallback()();
    } else {
      break;
    }
  }

  if (m_gameStatus != GameStatus::running) {
    return;
  }

  if (m_ball && hasBallFallenDown(*m_ball)) {
    m_events.push(Event([=]() { onBallFallDown(); }));
  }

  updatePickups(delta);

  Paddle paddleBackup = m_paddle;

  // dry run: simulate movement and detect if any collision could happened on
  // the way
  updatePaddleDynamics(m_paddle, delta);

  if (m_ball.has_value()) {
    Ball ballBackup = *m_ball;
    updateBallDynamics(*m_ball, delta);

    bool hasAnyCollision = detectBallCollisions(*m_ball, false) ||
                           collidesBallWithWorldBoundaries(*m_ball);

    // if ball has a potential collision, revert the state to initial and do
    // microstepping
    if (hasAnyCollision) {
      m_ball = ballBackup;
      m_paddle = paddleBackup;

      constexpr auto microstepsCount = 10;
      const auto microDelta = delta / microstepsCount;
      for (int i = 0; i < microstepsCount; i++) {
        updatePaddleDynamics(m_paddle, microDelta);
        updateBallDynamics(*m_ball, microDelta);
        correctBallAgainstWorldBoundaries(*m_ball);
        detectBallCollisions(*m_ball, true);
      }
    }
  }

  // update pickups and detect collisions
  // note: pickups are moving slow, we don't need to the maskarade above

  for (const auto& pickup : m_pickups) {
    if (SDL_HasIntersectionF(&pickup.body, &m_paddle.body)) {
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

  SDL_RenderSetViewport(app.getRenderer(), &viewport);

  // render tiles
  const auto tileTexture = app.getTexture("tile");
  for (const auto& entity : m_tileMap) {
    const auto& c = entity.color;
    SDL_SetRenderDrawColor(app.getRenderer(), c.r, c.g, c.b, c.a);

    const auto rect = worldToViewCoordinates(app, entity.body);

    SDL_RenderFillRect(app.getRenderer(), &rect);

    if (tileTexture) {
      SDL_SetTextureBlendMode(tileTexture, SDL_BLENDMODE_BLEND);
      SDL_RenderCopy(app.getRenderer(), tileTexture, NULL, &rect);
      SDL_SetTextureBlendMode(tileTexture, SDL_BLENDMODE_NONE);
    }
  }

  // render ball
  if (m_ball) {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.getRenderer(), c.r, c.g, c.b, c.a);

    SDL_FRect ballBody = m_ball->getBoundingRect();
    const auto rect = worldToViewCoordinates(app, ballBody);

    const auto ballTexture = app.getTexture("ball");
    if (ballTexture) {
      SDL_SetTextureBlendMode(ballTexture, SDL_BLENDMODE_BLEND);
      SDL_RenderCopy(app.getRenderer(), ballTexture, NULL, &rect);
      SDL_SetTextureBlendMode(ballTexture, SDL_BLENDMODE_NONE);
    } else {
      SDL_RenderFillRect(app.getRenderer(), &rect);
    }
  }

  // render paddle
  {
    const auto& c = Color::black;
    SDL_SetRenderDrawColor(app.getRenderer(), c.r, c.g, c.b, c.a);

    SDL_FRect body = m_paddle.body;
    const auto rect = worldToViewCoordinates(app, body);

    SDL_RenderFillRect(app.getRenderer(), &rect);
  }

  // render pickups
  for (const auto& entity : m_pickups) {
    const auto& c = entity.color;
    SDL_SetRenderDrawColor(app.getRenderer(), c.r, c.g, c.b, c.a);

    const auto rect = worldToViewCoordinates(app, entity.body);

    SDL_RenderFillRect(app.getRenderer(), &rect);
  }

  SDL_RenderSetViewport(app.getRenderer(), nullptr);
}

void
World::renderHUD(Application& app)
{
  if (m_gameStatus == GameStatus::running) {

    // render lives
    {
      const auto blackText = app.getCachedTextureForText(
        std::format("Lives: {}", m_gameState.remainingBalls));
      const auto textSize = sdl_helper::getsize(blackText);

      const auto ws = app.getWindowSize();
      SDL_Rect rect;
      rect.x = 5;
      rect.y = 5;
      rect.w = textSize.x;
      rect.h = textSize.y;

      SDL_RenderCopy(app.getRenderer(), blackText, NULL, &rect);
    }

    // render score
    {
      const auto blackText = app.getCachedTextureForText(
        std::format("Score: {}", m_gameState.score));
      const auto textSize = sdl_helper::getsize(blackText);

      const auto ws = app.getWindowSize();
      SDL_Rect rect;
      rect.x = ws.x - 5 - textSize.x;
      rect.y = 5;
      rect.w = textSize.x;
      rect.h = textSize.y;

      SDL_RenderCopy(app.getRenderer(), blackText, NULL, &rect);
    }
  }

  else {
    std::string textureName;

    if (m_gameStatus == GameStatus::initial_screen) {
      textureName = "arkanoid";
    }

    if (m_gameStatus == GameStatus::game_over) {
      textureName = "game_over";
    }
    if (m_gameStatus == GameStatus::you_won) {
      textureName = "you_won";
    }
    const auto overlayTexture = app.getTexture(textureName);
    SDL_SetTextureBlendMode(overlayTexture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(app.getRenderer(), overlayTexture, NULL, NULL);
    SDL_SetTextureBlendMode(overlayTexture, SDL_BLENDMODE_NONE);
  }
}

void
World::onKeyPressed(bool isKeyDown, SDL_Keysym key)
{
  SDL_Log("onKeyPressed: %d", isKeyDown);
  if (key.sym == SDLK_LEFT) {
    m_paddle.keys[ControllerKeys::move_left] = isKeyDown;
  }

  if (key.sym == SDLK_RIGHT) {
    m_paddle.keys[ControllerKeys::move_right] = isKeyDown;
  }

  if (key.sym == SDLK_SPACE && isKeyDown) {
    onReleaseBall();
  }

  if (key.sym == SDLK_r && isKeyDown) {
    onRestart();
  }

  if (key.sym == SDLK_RETURN && isKeyDown) {
    if (m_gameStatus == GameStatus::initial_screen) {
      onRestart();
    }
  }
}

void
World::updatePickups(std::chrono::microseconds delta)
{
  const auto elapsedSeconds = delta.count() / static_cast<float>(1000'000.0);

  for (auto it = m_pickups.begin(); it != m_pickups.end(); it++) {
    bool shouldErase = false;
    auto& pickup = *it;
    const auto initialBody = pickup.body;

    // update dynamics
    pickup.body.y += Constants::pickupFallSpeed * elapsedSeconds;

    // detect falling out of world
    if (pickup.body.y > Constants::worldHeight - pickup.body.h * 0.5) {
      m_events.push(Event([=]() { onPickupFallDown(pickup.id); }));
      continue;
    }

    // detect colliding paddle
    SDL_FRect pickupConvexHull = initialBody;
    pickupConvexHull.h = pickup.body.y - initialBody.y + initialBody.h;

    if (SDL_HasIntersectionF(&pickupConvexHull, &m_paddle.body)) {
      m_events.push(Event([=]() { onPickupPicked(pickup.id); }));
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

  // Keeps track of what were responses to collisions with different bodies
  // E.g. if ball hit two bodies and both would invertX, only invert it once
  std::pair<bool, bool> invertSpeed = { false, false };

  auto detectBallVsBodyCollision = [&](const SDL_FRect& body) -> bool {
    if (SDL_HasIntersectionF(&ballBody, &body)) {
      hasAnyCollision = true;

      invertSpeed = resolveBallSpeedCollisionAfter(ball, body);
      return true;
    }
    return false;
  };

  for (const auto& tile : m_tileMap) {
    if (detectBallVsBodyCollision(tile.body) && reportCollisions) {
      m_events.push(Event([=]() { onBallHitTile(tile.id); }));
    }
  }

  // against paddle

  bool hasPaddleCollision = false;
  {
    const auto body = m_paddle.body;
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
      const auto speed = m_paddle.getCurrentSpeed();
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
  m_pickups.push_back(pickup);
}

void
World::setWorldSpeed(float ratio)
{
  m_gameState.speed = ratio;
}

void
World::setBallSize(float ratio)
{
  if (m_ball) {
    m_ball->radius *= ratio;
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
  m_gameStatus = GameStatus::you_won;
  m_events.push(Event(std::chrono::seconds(10), [=]() { onRestart(); }));
}

void
World::onGameOver()
{
  SDL_Log("onGameOver");
  m_gameStatus = GameStatus::game_over;
  m_events.push(Event(std::chrono::seconds(10), [=]() { onRestart(); }));
}

void
World::onReleaseBall()
{
  SDL_Log("onReleaseBall");
  if (!m_ball.has_value()) {
    initializeBall();
  }
}

void
World::onBallHitTile(EntityID id)
{
  SDL_Log("onBallHitTile: %d", id);

  auto it = std::find_if(m_tileMap.begin(),
                         m_tileMap.end(),
                         [=](const Tile& tile) { return tile.id == id; });

  bool willBeDestroyed = false;
  // delete tile if is dead
  if (it != m_tileMap.end()) {
    it->lifes--;

    if (it->lifes == 0) {
      willBeDestroyed = true;

      m_gameState.score += Constants::rewardTileDestroyed;
    }
  }

  // when tile was destroyed and with some lower chance, generate special pickup
  if (willBeDestroyed && std::rand() % 5 == 0) {
    SDL_Point spawnPoint = { it->body.x, it->body.y };
    spawnRandomPickup(spawnPoint, it->color);
  }
  if (willBeDestroyed) {
    // destroy it
    m_tileMap.erase(it);
  }

  if (m_tileMap.empty()) {
    onLevelFinished();
  }
}

void
World::onBallFallDown()
{
  SDL_Log("onBallFallDown");
  m_ball.reset();

  m_gameState.score -= Constants::penaltyLostBall;
  if (m_gameState.remainingBalls == 0) {
    onGameOver();
  } else {
    m_gameState.remainingBalls--;
  }
}

void
World::onPickupPicked(EntityID pickupId)
{
  SDL_Log("onPickupPicked: %d", pickupId);
  auto it =
    std::find_if(m_pickups.begin(), m_pickups.end(), [=](const Pickup& entity) {
      return entity.id == pickupId;
    });

  // process events
  if (it != m_pickups.end()) {

    m_gameState.score += Constants::rewardPickupPicked;

    const auto& pickup = *it;
    switch (pickup.type) {
      case Pickup::Type::speedup: {
        setWorldSpeed(2.0);
        m_events.push(
          Event(std::chrono::seconds(10), [=]() { setWorldSpeed(1.0); }));
        break;
      }

      case Pickup::Type::slowdown: {
        setWorldSpeed(1.0);
        m_events.push(
          Event(std::chrono::seconds(10), [=]() { setWorldSpeed(1.0); }));
        break;
      }

      case Pickup::Type::change_ball_size: {

        constexpr float minimalRadius = 5.0;
        if (m_ball && m_ball->radius < minimalRadius)
          break;

        setBallSize(0.5);
        m_events.push(
          Event(std::chrono::seconds(10), [=]() { setBallSize(2.0); }));
        break;
      }

      case Pickup::Type::change_paddle_size: {
        // until restart
        m_paddle.body.w *= 2;
        m_paddle.body.w =
          std::clamp(m_paddle.body.w, 0.0f, Constants::worldWidth * 0.99f);

        if (Constants::worldWidth < (m_paddle.body.x + m_paddle.body.w)) {
          m_paddle.body.x = std::clamp(
            m_paddle.body.x, 0.0f, Constants::worldWidth - m_paddle.body.w);
        }
        break;
      }
    }
  }

  // delete pickup
  if (it != m_pickups.end()) {
    m_pickups.erase(it);
  }
}

void
World::onPickupFallDown(EntityID pickupId)
{
  SDL_Log("onPickupFallDown: %d", pickupId);
  auto it =
    std::find_if(m_pickups.begin(), m_pickups.end(), [=](const Pickup& entity) {
      return entity.id == pickupId;
    });
  // delete it
  if (it != m_pickups.end()) {
    m_pickups.erase(it);
  }
}

SDL_Rect
World::worldToViewCoordinates(Application& app, SDL_FRect units)
{
  SDL_Rect viewport;
  SDL_RenderGetViewport(app.getRenderer(), &viewport);

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
