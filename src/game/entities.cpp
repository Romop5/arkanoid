#include "entities.hpp"

#include "constants.hpp"

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