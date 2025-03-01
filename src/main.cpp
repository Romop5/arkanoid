#include <SDL.h>

#include "game/application.hpp"
#include "game/utils.hpp"
#include "game/world.hpp"
#include <chrono>
#include <format>
#include <functional>
#include <thread>

int
main(int argc, char* args[])
{
  using namespace std::chrono_literals;

  Application app;
  World world;

  auto lastFrame = std::chrono::high_resolution_clock::now();

  app.onRenderCallback = [&]() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto delta = now - lastFrame;

    if (!app.isStopped) {
      world.update(
        std::chrono::duration_cast<std::chrono::microseconds>(delta));
    }

    lastFrame = std::chrono::high_resolution_clock::now();

    world.render(app);
  };

  app.onSDLEventCallback = [&](const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      world.onKeyPressed(event.type == SDL_KEYDOWN, event.key.keysym);
    }
  };

  createApplication(app);
  return 0;
}
