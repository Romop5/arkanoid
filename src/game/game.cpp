#include "game.hpp"

void
Game::onRestart()
{
  initializeWorld();
}

void
Game::onLevelFinished()
{
  onRestart();
}

void
Game::onReleaseBall()
{
  if (!ball.has_value()) {
    initializeBall();
  }
}

void
Game::onBallHitTile(EntityID id)
{
  SDL_Log("onBallHitTile: %d", id);

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

  bool willBeDestroyed = false;
  // delete tile if is dead
  if (it != tileMap.end()) {
    it->lifes--;

    if (it->lifes == 0) {
      willBeDestroyed = true;
    }
  }

  // when tile was destroyed and with some lower chance, generate special pickup
  if (willBeDestroyed && std::rand() % 5 == 0) {
    static unsigned id = 0;
    Pickup pickup;
    pickup.id = id++;
    pickup.type = static_cast<Pickup::Type>(std::rand() % Pickup::Type::size);
    pickup.body = it->body;
    pickup.color = it->color;

    pickup.body.w *= 0.5;
    pickup.body.h *= 0.5;

    pickups.push_back(pickup);
  }
  if (willBeDestroyed) {
    // destroy it
    tileMap.erase(it);
  }

  if (tileMap.empty())
  {
    onLevelFinished();
  }
}

void
Game::onBallFallDown()
{
  SDL_Log("onBallFallDown");
  ball.reset();
}

void
Game::onPickupPicked(EntityID pickupId)
{
  SDL_Log("onPickupPicked: %d", pickupId);
  auto it =
    std::find_if(pickups.begin(), pickups.end(), [=](const Pickup& entity) {
      return entity.id == pickupId;
    });

  // process events
  if (it != pickups.end()) {
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
        setBallSize(0.5);
        events.push(
          Event(std::chrono::seconds(10), [=]() { setBallSize(2.0); }));
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
Game::onPickupFallDown(EntityID pickupId)
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