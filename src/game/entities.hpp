#pragma once

#include <SDL.h>
#include <bitset>

#include "constants.hpp"

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

  //! Current state of keyboard for actions (e.g. is move_left active)
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
