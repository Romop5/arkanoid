#pragma once

#include <SDL.h>
#include <functional>

#include "utils.hpp"

struct Application
{
  utils::RaiiOwnership<SDL_Window> window;
  utils::RaiiOwnership<SDL_Renderer> renderer;
  SDL_Surface* surface;

  std::function<void(const SDL_Event&)> onSDLEventCallback;
  std::function<void()> onRenderCallback;

  bool isStopped = { false };
};

void
createApplication(Application& application);