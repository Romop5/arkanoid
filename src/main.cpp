#include <SDL.h>

#include "game/application.hpp"
#include "game/utils.hpp"
#include "game/world.hpp"
#include <format>
#include <functional>
#include <chrono>
#include <thread>

// Screen dimension constants
constexpr int screenWidth = 640;
constexpr int screenHeight = 480;

void
createApplication(Application& application,
                  std::function<void()> renderCallback)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    throw std::runtime_error(
      std::format("SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
  }

  application.window = utils::make_raii_deleter<SDL_Window>(
    SDL_CreateWindow("Arkanoid",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     screenWidth,
                     screenHeight,
                     SDL_WINDOW_SHOWN),
    [](SDL_Window* window) -> void { SDL_DestroyWindow(window); });

  utils::throw_if_null(application.window.get(),
                       "Failed to initialize SDL Window");

  application.renderer = utils::make_raii_deleter<SDL_Renderer>(
    utils::throw_if_null(SDL_CreateRenderer(application.window.get(), -1, NULL),
                         "Failed to initialize renderer"),
    [](SDL_Renderer* renderer) { SDL_DestroyRenderer(renderer); });

  // Get window surface
  application.surface =
    utils::throw_if_null(SDL_GetWindowSurface(application.window.get()),
                         "Failed to obtain SDL's window surface");

  SDL_Event e;
  bool quit = false;
  while (quit == false) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        quit = true;
    }

    const auto frameBeggining = std::chrono::high_resolution_clock::now();

    SDL_SetRenderDrawColor(application.renderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(application.renderer.get());
    renderCallback();
    SDL_RenderPresent(application.renderer.get());

    // FPS lock on circa 60FPS
    using namespace std::chrono_literals;
    //std::this_thread::sleep_until(frameBeggining + 16ms);
  }

  SDL_Quit();
}

int
main(int argc, char* args[])
{
  using namespace std::chrono_literals;

  Application app;
  World world;

  auto lastFrame = std::chrono::high_resolution_clock::now();

  createApplication(app, [&]() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto delta = now - lastFrame;

    world.update(std::chrono::duration_cast<std::chrono::microseconds>(delta));

    lastFrame = std::chrono::high_resolution_clock::now();

    world.render(app);
  });

  return 0;
}
