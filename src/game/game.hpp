#pragma once

#include "constants.hpp"

class Game
{
    public:
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
}