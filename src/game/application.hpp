#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <functional>
#include <vector>

#include "text_manager.hpp"
#include "utils.hpp"

struct Application
{

public:
  void createApplication();
  void runLoop();

  void loadTexture(const std::string& name);
  void loadAssets(const std::string& assetDirectory);

  SDL_Renderer* getRenderer() const;

  //! Helper: get pixel size of app's window
  SDL_Point getWindowSize();

  SDL_Texture* getTexture(const std::string& name);

  //! Given text to be render, internally obtain a texture with rendered text
  SDL_Texture* getCachedTextureForText(const std::string& textureText);

  bool isStopped() const;

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

private:
  //! Handle to SDL (to deinitialize on destructor)
  utils::RaiiOwnership<void> m_sdlContext;

  //! Handle to application window
  utils::RaiiOwnership<SDL_Window> m_window;

  //! Handle to SDL's 2D renderer
  utils::RaiiOwnership<SDL_Renderer> m_renderer;

  TextManager m_textManager;

  //! Is application (render) running?
  bool m_isStopped = { false };

  std::unordered_map<std::string, SDL_Texture*> m_textures;
};
