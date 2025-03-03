#pragma once

#include <SDL.h>

namespace sdl_helper
{
    static SDL_Point getsize(SDL_Texture *texture) {
        SDL_Point size;
        SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
        return size;
    }
} // namespace sdl_helper