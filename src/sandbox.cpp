#include <SDL.h>
#include <cassert>
#include <iostream>

#include <game/world.hpp>

void
testCollisionStateDetection()
{

  {
    SDL_FRect rect = { 0, 0, 100, 100 };

    Ball ball;
    ball.radius = 10;
    ball.position = { 20, 20 };
    assert(ball.getCollisionStateForGivenRect(rect) ==
           Ball::CollisionState::full_inside);
  }

  {
    SDL_FRect rect = { 0, 0, 100, 100 };

    Ball ball;
    ball.radius = 10;
    ball.position = { 0, 0 };
    assert(ball.getCollisionStateForGivenRect(rect) ==
           Ball::CollisionState::left_top_corner);
  }

  {
    SDL_FRect rect = { 0, 0, 100, 100 };

    Ball ball;
    ball.radius = 10;
    ball.position = { 120, 120 };
    assert(ball.getCollisionStateForGivenRect(rect) ==
           Ball::CollisionState::no_collision);
  }

  {
    SDL_FRect rect = { 0, 0, 100, 100 };

    Ball ball;
    ball.radius = 10;
    ball.position = { 100, 60 };
    assert(ball.getCollisionStateForGivenRect(rect) ==
           Ball::CollisionState::from_right);
  }
}

int
main(int argc, char* args[])
{
  testCollisionStateDetection();
  std::cout << "end" << std::endl;
  return 0;
}