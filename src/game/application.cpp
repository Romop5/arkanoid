#include <chrono>
#include <format>
#include <thread>
#include <SDL_image.h>
#include <filesystem>

#include "application.hpp"
#include "constants.hpp"

void
createApplication(Application& application)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    throw std::runtime_error(
      std::format("SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
  }

  application.window = utils::make_raii_deleter<SDL_Window>(
    SDL_CreateWindow("Arkanoid",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     Constants::screenWidth,
                     Constants::screenHeight,
                     SDL_WINDOW_SHOWN),
    [](SDL_Window* window) -> void { SDL_DestroyWindow(window); });

  utils::throw_if_null(application.window.get(),
                       "Failed to initialize SDL Window");

  application.renderer = utils::make_raii_deleter<SDL_Renderer>(
    utils::throw_if_null(
      SDL_CreateRenderer(application.window.get(),
                         -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      "Failed to initialize renderer"),
    [](SDL_Renderer* renderer) { SDL_DestroyRenderer(renderer); });

  // Get window surface
  application.surface =
    utils::throw_if_null(SDL_GetWindowSurface(application.window.get()),
                         "Failed to obtain SDL's window surface");

  application.onInitCallback();

  SDL_Event e;
  bool quit = false;
  while (quit == false) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        quit = true;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        application.isStopped = !application.isStopped;
      }

      if (application.onSDLEventCallback) {
        application.onSDLEventCallback(e);
      }
    }

    const auto frameBeggining = std::chrono::high_resolution_clock::now();

    SDL_SetRenderDrawColor(application.renderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(application.renderer.get());
    application.onRenderCallback();
    SDL_RenderPresent(application.renderer.get());

    // FPS lock on circa 60FPS
    using namespace std::chrono_literals;
    std::this_thread::sleep_until(frameBeggining + 16ms);
  }

  SDL_Quit();
}

void
loadTexture(Application& application, const std::string& name)
{
  SDL_Texture* texture =  IMG_LoadTexture(application.renderer.get(), name.c_str());

  utils::throw_if_null(texture, std::string("Failed to load texture: ") + name);

  const auto fileName = std::filesystem::path(name).filename().string();

  application.textures[fileName] = texture;
}
