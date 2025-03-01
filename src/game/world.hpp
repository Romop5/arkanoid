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

namespace Color {
static constexpr auto white = SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF };
static constexpr auto red = SDL_Color{ 0xFF, 0x00, 0x00, 0xFF };
static constexpr auto blue = SDL_Color{ 0x00, 0xFF, 0x00, 0xFF };
static constexpr auto green = SDL_Color{ 0x00, 0x00, 0xFF, 0xFF };
static constexpr auto gray = SDL_Color{ 0x33, 0x33, 0x33, 0xFF };
static constexpr auto black = SDL_Color{ 0x00, 0x00, 0x00, 0xFF };
} // namespace color

using TileID = unsigned;
struct Tile
{
  TileID id = -1;

  //! Position with width/height (in world units)
  SDL_FRect body = { 0, 0, 0, 0 };

  //! Color when drawing tile
  SDL_Color color = Color::white;

  //! How many count of collions remains before dying
  std::uint8_t lifes = 1;
};

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

/**
 * @brief Logical definition of the world and its entities
 */
class World
{
public:
  using Event = std::function<void()>;
public:
  World();

  void update(std::chrono::microseconds delta);
  void render(Application& app);

protected:
  void updateBallDynamics(Ball& ball, std::chrono::microseconds delta);
  bool collidesBallWithWorldBoundaries(Ball& ball);
  void correctBallAgainstWorldBoundaries(Ball& ball);
  bool detectBallCollisions(Ball& ball, bool reportCollisions);
  std::pair<bool, bool> resolveBallSpeedCollisionAfter(SDL_FRect rect);

protected:
  //! Event: ball hit tile
  void onBallHitTile(TileID id);

  SDL_Rect worldToViewCoordinates(Application&, SDL_FRect units);

private:
  static constexpr unsigned width = 1000; // of world units
  static constexpr unsigned height = 750; // of world units

  static constexpr unsigned maxTilesX = 10;
  static constexpr unsigned maxTilesY = 10;

  static constexpr unsigned tileWidth = width / maxTilesX;   // of world units
  static constexpr unsigned tileHeight = height / maxTilesY; // of world units

  // the world space should be able to cover multiples of tiles
  static_assert(width % tileWidth == 0);
  static_assert(height % tileHeight == 0);

  std::vector<Tile> tileMap;

  Ball ball;

  //! FIFO of events
  std::queue<Event> events;
};