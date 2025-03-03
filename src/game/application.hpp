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

  void loadTexture(const std::string& name);

  void loadAssets(const std::string& assetDirectory);

  SDL_Texture* getTextureForText(const std::string& textureText);

  SDL_Point getWindowSize();

protected:
  SDL_Texture* createTextureFromText(const std::string& textureText,
                                     SDL_Color textColor);

public:
  //! Handle to application window
  utils::RaiiOwnership<SDL_Window> window;

  //! Handle to SDL's 2D renderer
  utils::RaiiOwnership<SDL_Renderer> renderer;

  SDL_Surface* surface;

  //! Called when event arises in SDL polling mechanism
  std::function<void(const SDL_Event&)> onSDLEventCallback;

  //! Called after SDL and Renderer was initialized
  std::function<void()> onInitCallback;

  //! Call each frame after clear and before swap (present)
  std::function<void()> onRenderCallback;

  //! Is application (render) running?
  bool isStopped = { false };

  std::unordered_map<std::string, SDL_Texture*> textures;

  utils::RaiiOwnership<TTF_Font> font;

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
    void removeUnused();

    public:
    std::unordered_map<std::string, TextTexture> textures;
  };

  TextManager textManager;
};
