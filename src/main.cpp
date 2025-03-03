#include <SDL.h>

#include <SDL_image.h>
#include <chrono>
#include <format>
#include <functional>
#include <thread>

#include "game/application.hpp"
#include "game/sdl_helper.hpp"
#include "game/utils.hpp"
#include "game/world.hpp"

namespace {
void
renderApplicationOverlay(Application& app)
{
  // if stopped, add overlay
  if (app.isStopped) {
    // temporal gradient of apha for overlay
    unsigned alpha = 50 + (SDL_GetTicks() / 30) % 20;
    SDL_SetRenderDrawBlendMode(app.renderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(app.renderer.get(), 255, 255, 255, alpha);
    SDL_RenderFillRect(app.renderer.get(), NULL);
    SDL_SetRenderDrawBlendMode(app.renderer.get(), SDL_BLENDMODE_NONE);
  }
}
} // namespace

int
main(int argc, char* args[])
{
  using namespace std::chrono_literals;

  Application app;
  World world;

  auto lastFrame = std::chrono::high_resolution_clock::now();

  app.onInitCallback = [&]() { loadAssets(app, "assets"); };
  app.onRenderCallback = [&]() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto delta = now - lastFrame;

    if (!app.isStopped) {
      world.update(
        std::chrono::duration_cast<std::chrono::microseconds>(delta));
    }

    lastFrame = std::chrono::high_resolution_clock::now();

    world.render(app);

    renderApplicationOverlay(app);
  };

  app.onSDLEventCallback = [&](const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      world.onKeyPressed(event.type == SDL_KEYDOWN, event.key.keysym);
    }
  };

  try {
    createApplication(app);
  } catch (const std::runtime_error& error) {
    SDL_ShowSimpleMessageBox(
      SDL_MESSAGEBOX_ERROR, "Fatal Error", error.what(), nullptr);
  }
  return 0;
}
