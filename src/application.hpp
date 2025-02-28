#pragma once

#include <SDL.h>
#include "utils.hpp"

struct Application
{
  utils::RaiiOwnership<SDL_Window> window;
  utils::RaiiOwnership<SDL_Renderer> renderer;
  SDL_Surface* surface;
};