#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <functional>
#include <vector>

#include "utils.hpp"

struct Application
{

public:
  void createApplication();
  void runLoop();

  void loadTexture(const std::string& name);
  void loadAssets(const std::string& assetDirectory);

  //! Given text to be render, internally obtain a texture with rendered text
  SDL_Texture* getCachedTextureForText(const std::string& textureText);

  //! Helper: get pixel size of app's window
  SDL_Point getWindowSize();

public:
  //! Handle to SDL (to deinitialize on destructor)
  utils::RaiiOwnership<void> sdlContext;

  //! Handle to application window
  utils::RaiiOwnership<SDL_Window> window;

  //! Handle to SDL's 2D renderer
  utils::RaiiOwnership<SDL_Renderer> renderer;

public:
  //! Called when event arises in SDL polling mechanism
  std::function<void(const SDL_Event&)> onSDLEventCallback;

  //! Called after SDL and Renderer was initialized
  std::function<void()> onInitCallback;

  //! Call each frame after clear and before swap (present)
  std::function<void()> onRenderCallback;

protected:
  void initializeSDL();

  void initializeWindowAndRenderer();

  SDL_Texture* createTextureFromText(TTF_Font* font,
                                     const std::string& textureText,
                                     SDL_Color textColor);

public:
  //! Is application (render) running?
  bool isStopped = { false };

  std::unordered_map<std::string, SDL_Texture*> textures;

  //! @brief Shitty manager for text textures (this architecture sucks, but I am
  //! in hurry atm)
  struct TextManager
  {
    struct TextTexture
    {
      std::chrono::high_resolution_clock::time_point lastUsed;
      SDL_Texture* texture;
    };

  public:
    void initialize();

    void removeUnused();

  public:
    utils::RaiiOwnership<TTF_Font> font;

    std::unordered_map<std::string, TextTexture> textures;
  };

  TextManager textManager;
};
