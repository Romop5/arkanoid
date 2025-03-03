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
#include <optional>
#include <queue>

#include "application.hpp"
#include "ball.hpp"
#include "constants.hpp"
#include "entities.hpp"
#include "event.hpp"

enum GameStatus
{
  initial_screen,
  running,
  you_won,
  game_over
};

struct GameState
{
  //! Game speed rate (for all moving objects)
  float speed{ 1.0f };

  //! How many balls can player shoot again
  unsigned remainingBalls{ 2 };

  //! Current score
  int score{ 0 };
};

/**
 * @brief Logical definition of the world and its entities
 */
class World
{
public:
  void update(std::chrono::microseconds delta);
  void render(Application& app);
  void onKeyPressed(bool isKeyDown, SDL_Keysym key);

protected:
  void initializeWorld();
  void initializeBall();
  void initializePaddle();

  void renderEntities(Application& app);
  void renderHUD(Application& app);

  void updatePickups(std::chrono::microseconds delta);
  void updatePaddleDynamics(Paddle& paddle, std::chrono::microseconds delta);
  void updateBallDynamics(Ball& ball, std::chrono::microseconds delta);

  bool hasBallFallenDown(Ball& ball);
  bool collidesBallWithWorldBoundaries(Ball& ball);

  void correctBallAgainstWorldBoundaries(Ball& ball);
  bool detectBallCollisions(Ball& ball, bool reportCollisions);

  //! Returns pair <invertSpeedX, invertSpeedY> to adjust speed after collision
  std::pair<bool, bool> resolveBallSpeedCollisionAfter(Ball& ball,
                                                       SDL_FRect rect);

  //! Spawn a random tile at position
  void spawnRandomTile(unsigned x, unsigned y);

  //! Spawn a random pickup
  void spawnRandomPickup(SDL_Point position, SDL_Color color);

protected:
  void setWorldSpeed(float ratio);
  void setBallSize(float ratio);

  //! Internal Event: reinitialize the game
  void onRestart();

  //! Event: When game finishes (all tiles are destroyed)
  void onLevelFinished();

  //! Event: When game is over (all balls are lost)
  void onGameOver();

  //! Event: release ball from paddle if possible
  void onReleaseBall();

  //! Event: ball hit tile
  void onBallHitTile(EntityID tileId);

  //! Event: ball fall down the world
  void onBallFallDown();

  //! Event: pickup picked by player
  void onPickupPicked(EntityID pickupId);

  //! Event: pickup was not caught by paddle and fall down the world
  void onPickupFallDown(EntityID pickupId);

  SDL_Rect worldToViewCoordinates(Application&, SDL_FRect units);

private:
  //! Static tiles
  std::vector<Tile> m_tileMap;

  //! Dynamic objects: pickups
  std::vector<Pickup> m_pickups;

  //! Dynamic object: moving ball
  std::optional<Ball> m_ball;

  //! User's paddle
  Paddle m_paddle;

  //! Event queue, sorted w.r.t. deadline time
  std::priority_queue<Event> m_events;

  GameStatus m_gameStatus{ GameStatus::initial_screen };

  //! Defines parameters of the level 
  GameState m_gameState;
};