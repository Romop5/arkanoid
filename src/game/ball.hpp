#pragma once

#include <SDL.h>
#include <array>
#include <bitset>
#include <cassert>

struct Ball
{
  SDL_FPoint position = { 0, 0 };
  SDL_FPoint speed = { 0, 0 }; // units/second
  float radius = 50.0;         // world units

  //! Returns bounding box (rectangle)
  SDL_FRect getBoundingRect();

  enum Corner
  {
    top_left = 0,
    top_right = 1,
    bottom_right = 2,
    bottom_left = 3
  };

  //! Return bounding rectangle as an array of corners (2D points)
  std::array<SDL_FPoint, 4> getBoundingRectCorners();

  //! Collision states that can happen between ball and rectangle (e.g. tile)
  enum class CollisionState
  {
    no_collision,
    from_left,
    from_right,
    from_above,
    from_bottom,
    left_top_corner,
    right_top_corner,
    left_bottom_corner,
    right_bottom_corner,
    full_inside,
  };

  //! If given corner is inside, which scenario probably happened?
  CollisionState cornerToClosestBoundaryDistance(SDL_FPoint corner,
                                                 SDL_FRect rect);

  //! Evaluate collision between this ball and rect
  CollisionState getCollisionStateForGivenRect(SDL_FRect rect);
};