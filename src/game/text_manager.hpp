#pragma once

#include <chrono>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>

#include "utils.hpp"

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

  bool hasTexture(const std::string& text) const;
  void addTexture(const std::string& text, SDL_Texture* texture);
  SDL_Texture* getTexture(const std::string& text) const;

  TTF_Font* getFont() const;

private:
  utils::RaiiOwnership<TTF_Font> font;

  std::unordered_map<std::string, TextTexture> textures;
};