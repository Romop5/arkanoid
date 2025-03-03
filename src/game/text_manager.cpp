#include "text_manager.hpp"

#include <cassert>
#include <filesystem>

void
TextManager::initialize()
{
  const auto fontPath = std::filesystem::absolute("assets/font.ttf").string();
  assert(std::filesystem::exists(fontPath));

  font = utils::make_raii_deleter<TTF_Font>(
    utils::throw_if_null(TTF_OpenFont(fontPath.c_str(), 28),
                         std::string("Failed to open TTF font: ") + fontPath),
    [](TTF_Font* font) { TTF_CloseFont(font); });
}

void
TextManager::removeUnused()
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

bool
TextManager::hasTexture(const std::string& text) const
{
    return textures.count(text);
}

void
TextManager::addTexture(const std::string& text, SDL_Texture* texture)
{
  textures[text] =
    TextTexture{ std::chrono::high_resolution_clock::now(), texture };
}

SDL_Texture*
TextManager::getTexture(const std::string& text) const
{
  return textures.at(text).texture;
}

TTF_Font*
TextManager::getFont() const
{
  return font.get();
}
