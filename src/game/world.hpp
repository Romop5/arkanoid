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
#include "constants.hpp"
#include <optional>
#include <queue>

using EntityID = unsigned;
struct Tile
{
  EntityID id = -1;

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

  float getCurrentSpeed() const;
};

struct Pickup
{
public:
  enum Type
  {
    speedup,
    slowdown,
    change_ball_size,
    change_paddle_size,
    size
  };

public:
  EntityID id = -1;
  Type type;
  SDL_FRect body;
  SDL_Color color = Color::white;
};

struct Event
{
  using EventCallback = std::function<void()>;

  Event() = default;

  Event(std::chrono::milliseconds ms, EventCallback callback)
    : deadline(std::chrono::high_resolution_clock::now() + ms)
    , callback(std::move(callback))
  {
  }

  Event(EventCallback callback)
    : Event(std::chrono::milliseconds(0), callback)
  {
  }

  std::chrono::high_resolution_clock::time_point deadline;
  EventCallback callback;

  bool operator<(const Event& other) const
  {
    return this->deadline > other.deadline;
  }
};

/**
 * @brief Logical definition of the world and its entities
 */
class World
{
public:
  World();

  void initializeWorld();
  void initializeBall();

  void update(std::chrono::microseconds delta);
  void render(Application& app);

public:
  void onKeyPressed(bool isKeyDown, SDL_Keysym key);

protected:
  void updatePickups(std::chrono::microseconds delta);
  void updatePaddleDynamics(Paddle& paddle, std::chrono::microseconds delta);
  void updateBallDynamics(Ball& ball, std::chrono::microseconds delta);

  bool hasBallFallenDown(Ball& ball);
  bool collidesBallWithWorldBoundaries(Ball& ball);

  void correctBallAgainstWorldBoundaries(Ball& ball);
  bool detectBallCollisions(Ball& ball, bool reportCollisions);
  std::pair<bool, bool> resolveBallSpeedCollisionAfter(Ball& ball,
                                                       SDL_FRect rect);

protected:
  void setWorldSpeed(float ratio);
  void setBallSize(float ratio);

  //! Event: reinitialize the game
  void onRestart();
  
  void onLevelFinished();

  //! Event: release ball from paddle if possible
  void onReleaseBall();

  //! Event: ball hit tile
  void onBallHitTile(EntityID tileId);

  //! Event: ball hit tile
  void onBallFallDown();

  void onPickupPicked(EntityID pickupId);
  void onPickupFallDown(EntityID pickupId);

  SDL_Rect worldToViewCoordinates(Application&, SDL_FRect units);

private:
  //! Static tiles
  std::vector<Tile> tileMap;

  //! Dynamic objects: pickups
  std::vector<Pickup> pickups;

  //! Dynamic object: moving ball
  std::optional<Ball> ball;

  //! User's paddle
  Paddle paddle;

  //! FIFO of events
  std::priority_queue<Event> events;

  float speed{ 1.0f };

  bool isGameOver{ false };

  //! How many balls can player shoot again
  unsigned remainingBalls{ 2 };
};