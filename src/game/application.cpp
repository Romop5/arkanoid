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

  m_textManager.initialize();

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
        m_isStopped = !m_isStopped;
      }

      if (onSDLEventCallback) {
        onSDLEventCallback(e);
      }
    }

    const auto frameBeggining = std::chrono::high_resolution_clock::now();

    const auto clearColor = Color::white;
    SDL_SetRenderDrawColor(
      m_renderer.get(), clearColor.r, clearColor.g, clearColor.b, clearColor.a);

    SDL_RenderClear(m_renderer.get());
    onRenderCallback();
    SDL_RenderPresent(m_renderer.get());

    // FPS lock on circa 60FPS
    using namespace std::chrono_literals;
    std::this_thread::sleep_until(frameBeggining + 16ms);

    // clean-up text render cache
    m_textManager.removeUnused();
  }
}

void
Application::loadTexture(const std::string& name)
{
  SDL_Log("Loading texture: %s", name.c_str());

  SDL_Texture* texture =
    utils::throw_if_null(IMG_LoadTexture(m_renderer.get(), name.c_str()),
                         std::string("Failed to load texture: ") + name);

  const auto fileName = std::filesystem::path(name).stem().string();
  m_textures[fileName] = texture;
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
  if (!m_textManager.hasTexture(textureText)) {
    m_textManager.addTexture(
      textureText,
      createTextureFromText(m_textManager.getFont(), textureText, Color::black));
  }

  return m_textManager.getTexture(textureText);
}

bool
Application::isStopped() const
{
  return m_isStopped;
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
  m_sdlContext = utils::make_raii_action([]() { SDL_Quit(); });

  if (TTF_Init() < 0) {
    throw std::runtime_error(std::format(
      "Failed to initialize SDL's TTF module: %s\n", TTF_GetError()));
  }
}

void
Application::initializeWindowAndRenderer()
{
  m_window = utils::make_raii_deleter<SDL_Window>(
    SDL_CreateWindow("Arkanoid",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     Constants::screenWidth,
                     Constants::screenHeight,
                     SDL_WINDOW_SHOWN),
    [](SDL_Window* window) -> void { SDL_DestroyWindow(window); });

  utils::throw_if_null(m_window.get(), "Failed to initialize SDL Window");

  m_renderer = utils::make_raii_deleter<SDL_Renderer>(
    utils::throw_if_null(
      SDL_CreateRenderer(
        m_window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
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
    SDL_CreateTextureFromSurface(m_renderer.get(), textSurface.get()),
    "Unable to create texture from rendered text!");

  return texture;
}

SDL_Point
Application::getWindowSize()
{
  SDL_Surface* surface =
    utils::throw_if_null(SDL_GetWindowSurface(m_window.get()),
                         "Failed to obtain SDL's window surface");
  return SDL_Point(surface->w, surface->h);
}

SDL_Texture*
Application::getTexture(const std::string& name)
{
  assert(m_textures.count(name));
  return m_textures[name];
}

SDL_Renderer*
Application::getRenderer() const
{
  return utils::throw_if_null(m_renderer.get(), "renderer is nullptr");
}
