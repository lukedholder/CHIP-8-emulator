#include "platform.hpp"

#include <stdexcept>
#include <string>

Platform::Platform(const char* title,
                    int windowWidth, int windowHeight,
                    int textureWidth, int textureHeight) {
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                windowWidth, windowHeight,
                                SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        const std::string error = SDL_GetError();   // save error before SDL quits
        cleanup();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        const std::string error = SDL_GetError();
        cleanup();
        throw std::runtime_error("SDL_CreateRenderer failed: " + error);
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                textureWidth, textureHeight);
    if (texture == nullptr) {
        const std::string error = SDL_GetError();
        cleanup();
        throw std::runtime_error("SDL_CreateTexture failed: " + error);
    }

    pitch = textureWidth * static_cast<int>(sizeof(uint32_t));
}

Platform::~Platform() {
    cleanup();
}

void Platform::cleanup() noexcept {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Platform::update(const uint32_t* pixels) {
    SDL_UpdateTexture(texture, nullptr, pixels, pitch);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

bool Platform::processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            return false;
        }
    }

    return true;
}
