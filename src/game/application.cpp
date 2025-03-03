#include <SDL_image.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <thread>
#include <utility>

#include "application.hpp"
#include "constants.hpp"
#include <assert.h>

void
Application::createApplication()
{
  initializeSDL();

  initializeWindowAndRenderer();

  textManager.initialize();

  onInitCallback();
}

void
Application::runLoop()
{
  SDL_Event e;
  bool quit = false;
  while (quit == false) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        quit = true;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        isStopped = !isStopped;
      }

      if (onSDLEventCallback) {
        onSDLEventCallback(e);
      }
    }

    const auto frameBeggining = std::chrono::high_resolution_clock::now();

    const auto clearColor = Color::white;
    SDL_SetRenderDrawColor(
      renderer.get(), clearColor.r, clearColor.g, clearColor.b, clearColor.a);

    SDL_RenderClear(renderer.get());
    onRenderCallback();
    SDL_RenderPresent(renderer.get());

    // FPS lock on circa 60FPS
    using namespace std::chrono_literals;
    std::this_thread::sleep_until(frameBeggining + 16ms);

    // clean-up text render cache
    textManager.removeUnused();
  }
}

void
Application::loadTexture(const std::string& name)
{
  SDL_Log("Loading texture: %s", name.c_str());

  SDL_Texture* texture =
    utils::throw_if_null(IMG_LoadTexture(renderer.get(), name.c_str()),
                         std::string("Failed to load texture: ") + name);

  const auto fileName = std::filesystem::path(name).stem().string();
  textures[fileName] = texture;
}

void
Application::loadAssets(const std::string& assetDirectory)
{
  for (auto const& dir_entry :
       std::filesystem::directory_iterator{ assetDirectory }) {
    if (!dir_entry.is_regular_file()) {
      continue;
    }

    if (dir_entry.path().extension() == ".png") {
      loadTexture(dir_entry.path().string());
    }
  }
}

SDL_Texture*
Application::getCachedTextureForText(const std::string& textureText)
{
  // create and cache texture for given text
  if (!textManager.textures.count(textureText)) {
    TextManager::TextTexture textTexture;

    textTexture.lastUsed = std::chrono::high_resolution_clock::now();
    textTexture.texture =
      createTextureFromText(textManager.font.get(), textureText, Color::black);

    textManager.textures[textureText] = textTexture;
  }

  return textManager.textures.at(textureText).texture;
}

void
Application::initializeSDL()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    throw std::runtime_error(
      std::format("SDL could not initialize! SDL_Error: %s\n", SDL_GetError()));
  }

  // since now SDL is initialized, we need to free it once application is
  // destroyed
  sdlContext = utils::make_raii_action([]() { SDL_Quit(); });

  if (TTF_Init() < 0) {
    throw std::runtime_error(std::format(
      "Failed to initialize SDL's TTF module: %s\n", TTF_GetError()));
  }
}

void
Application::initializeWindowAndRenderer()
{
  window = utils::make_raii_deleter<SDL_Window>(
    SDL_CreateWindow("Arkanoid",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     Constants::screenWidth,
                     Constants::screenHeight,
                     SDL_WINDOW_SHOWN),
    [](SDL_Window* window) -> void { SDL_DestroyWindow(window); });

  utils::throw_if_null(window.get(), "Failed to initialize SDL Window");

  renderer = utils::make_raii_deleter<SDL_Renderer>(
    utils::throw_if_null(
      SDL_CreateRenderer(
        window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      "Failed to initialize renderer"),
    [](SDL_Renderer* renderer) { SDL_DestroyRenderer(renderer); });
}

//! Adapted from: https://lazyfoo.net/tutorials/SDL/16_true_type_fonts/index.php
SDL_Texture*
Application::createTextureFromText(TTF_Font* font,
                                   const std::string& textureText,
                                   SDL_Color textColor)
{
  assert(font != nullptr);

  SDL_Texture* texture = nullptr;

  // Render text surface
  utils::RaiiOwnership<SDL_Surface> textSurface =
    utils::make_raii_deleter<SDL_Surface>(
      utils::throw_if_null(
        TTF_RenderText_Solid(font, textureText.c_str(), textColor),
        "Unable to render text surface!"),
      [](SDL_Surface* surface) { SDL_FreeSurface(surface); });

  // Create texture from surface pixels
  texture = utils::throw_if_null(
    SDL_CreateTextureFromSurface(renderer.get(), textSurface.get()),
    "Unable to create texture from rendered text!");

  return texture;
}

SDL_Point
Application::getWindowSize()
{
  SDL_Surface* surface = utils::throw_if_null(SDL_GetWindowSurface(window.get()),
                                 "Failed to obtain SDL's window surface");
  return SDL_Point(surface->w, surface->h);
}

void
Application::TextManager::initialize()
{
  const auto fontPath = std::filesystem::absolute("assets/font.ttf").string();
  assert(std::filesystem::exists(fontPath));

  font = utils::make_raii_deleter<TTF_Font>(
    utils::throw_if_null(TTF_OpenFont(fontPath.c_str(), 28),
                         std::string("Failed to open TTF font: ") + fontPath),
    [](TTF_Font* font) { TTF_CloseFont(font); });
}

void
Application::TextManager::removeUnused()
{
  using namespace std::chrono_literals;
  const auto maxUnusedDuration = 5s;

  const auto now = std::chrono::high_resolution_clock::now();
  for (auto it = textures.begin(); it != textures.end(); it++) {

    const auto lastUsed = it->second.lastUsed;
    if ((lastUsed + maxUnusedDuration) < now) {
      it = textures.erase(it);
    }

    if (it == textures.end()) {
      break;
    }
  }
}
