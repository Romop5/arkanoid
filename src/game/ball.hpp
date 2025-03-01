#pragma once

struct Ball
{
  SDL_FPoint position = { 0, 0 };
  SDL_FPoint speed = { 0, 0 }; // units/second
  float radius = 50.0;

  SDL_FRect getBoundingRect()
  {
    SDL_FRect ballBody;
    ballBody.x = position.x - radius;
    ballBody.y = position.y - radius;
    ballBody.w = 2.0 * radius;
    ballBody.h = 2.0 * radius;
    return ballBody;
  }

  enum Corner
  {
    top_left = 0,
    top_right = 1,
    bottom_right = 2,
    bottom_left = 3
  };

  std::array<SDL_FPoint, 4> getBoundingRectCorners()
  {
    const auto b = getBoundingRect();
    return { SDL_FPoint{ b.x, b.y },   // top-left
             { b.x + b.w, b.y },       // top-right
             { b.x + b.w, b.y + b.h }, // bottom-right
             { b.x, b.y + b.h } };     // bottom-left
  }

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

  CollisionState cornerToClosestBoundaryDistance(SDL_FPoint corner,
                                                 SDL_FRect rect)
  {
    const auto distanceToUpperBoundary = std::abs(corner.y - rect.y);
    const auto distanceToBottomBoundary =
      std::abs(corner.y - (rect.y + rect.h));

    const auto distanceToLeftBoundary = std::abs(corner.x - rect.x);
    const auto distanceToRightBoundary = std::abs(corner.x - (rect.x + rect.w));

    const auto closestX =
      std::min(distanceToLeftBoundary, distanceToRightBoundary);
    const auto closestY =
      std::min(distanceToUpperBoundary, distanceToBottomBoundary);

    if (closestX < closestY) {
      return (distanceToLeftBoundary < distanceToRightBoundary)
               ? CollisionState::from_left
               : CollisionState::from_right;
    } else {

      return (distanceToUpperBoundary < distanceToBottomBoundary)
               ? CollisionState::from_above
               : CollisionState::from_bottom;
    }
  }

  CollisionState getCollisionStateForGivenRect(SDL_FRect rect)
  {
    const auto body = getBoundingRect();
    if (!SDL_HasIntersectionF(&rect, &body)) {
      return CollisionState::no_collision;
    }

    const auto corners = getBoundingRectCorners();
    std::bitset<4> isCornerInside;

    for (int i = 0; i < 4; i++) {
      isCornerInside[i] = SDL_PointInFRect(&corners[i], &rect);
    }

    const auto cornersInsideCount = isCornerInside.count();

    // false collision ? or rounding troubles
    if (cornersInsideCount == 0) {
      return CollisionState::no_collision;
    }

    // impossible situation for two rectangles
    assert(cornersInsideCount != 3);

    switch (cornersInsideCount) {
      case 4:
        return CollisionState::full_inside;
      case 1: {
        int index = 0;
        for (int i = 0; i < 4; i++) {
          if (isCornerInside[i]) {
            index = i;
            break;
          }
        }

        return cornerToClosestBoundaryDistance(corners[index], rect);
      }

      case 2: {
        if (isCornerInside[Corner::top_left] &&
            isCornerInside[Corner::top_right])
          return CollisionState::from_bottom;

        if (isCornerInside[Corner::top_left] &&
            isCornerInside[Corner::bottom_left])
          return CollisionState::from_right;

        if (isCornerInside[Corner::top_right] &&
            isCornerInside[Corner::bottom_right])
          return CollisionState::from_left;

        if (isCornerInside[Corner::bottom_right] &&
            isCornerInside[Corner::bottom_left])
          return CollisionState::from_above;
      }
      default:
        assert(cornersInsideCount != 0 && cornersInsideCount != 3);
    }

    assert(false);
    return CollisionState::no_collision;
  }
};