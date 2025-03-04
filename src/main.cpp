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
  if (app.isStopped()) {
    // temporal gradient of apha for overlay
    unsigned alpha = 50 + (SDL_GetTicks() / 30) % 20;
    SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(app.getRenderer(), 255, 255, 255, alpha);
    SDL_RenderFillRect(app.getRenderer(), NULL);
    SDL_SetRenderDrawBlendMode(app.getRenderer(), SDL_BLENDMODE_NONE);

    const auto blackText = app.getCachedTextureForText(std::format("Paused"));
    const auto textSize = sdl_helper::getTextureSize(blackText);

    const auto ws = app.getWindowSize();
    SDL_Rect rect;
    rect.x = (ws.x - textSize.x) / 2;
    rect.y = (ws.y - textSize.y) / 2;
    rect.w = textSize.x;
    rect.h = textSize.y;

    SDL_RenderCopy(app.getRenderer(), blackText, NULL, &rect);
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

  app.onInitCallback = [&]() { app.loadAssets("assets"); };
  app.onRenderCallback = [&]() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto delta = now - lastFrame;

    if (!app.isStopped()) {
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
    app.createApplication();
    app.runLoop();
  } catch (const std::runtime_error& error) {
    SDL_ShowSimpleMessageBox(
      SDL_MESSAGEBOX_ERROR, "Fatal Error", error.what(), nullptr);
  }
  return 0;
}
