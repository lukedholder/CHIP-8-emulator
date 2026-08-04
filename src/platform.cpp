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

bool Platform::processInput(std::array<uint8_t, 16>& keys) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            return false;
        }
    }

    // CHIP-8 keypad -> physical keyboard positions:
    //   1 2 3 C        1 2 3 4
    //   4 5 6 D        Q W E R
    //   7 8 9 E        A S D F
    //   A 0 B F        Z X C V
    static constexpr SDL_Scancode KEYMAP[16] = {
        SDL_SCANCODE_X,  // 0
        SDL_SCANCODE_1,  // 1
        SDL_SCANCODE_2,  // 2
        SDL_SCANCODE_3,  // 3
        SDL_SCANCODE_Q,  // 4
        SDL_SCANCODE_W,  // 5
        SDL_SCANCODE_E,  // 6
        SDL_SCANCODE_A,  // 7
        SDL_SCANCODE_S,  // 8
        SDL_SCANCODE_D,  // 9
        SDL_SCANCODE_Z,  // A
        SDL_SCANCODE_C,  // B
        SDL_SCANCODE_4,  // C
        SDL_SCANCODE_R,  // D
        SDL_SCANCODE_F,  // E
        SDL_SCANCODE_V,  // F
    };

    const Uint8* state = SDL_GetKeyboardState(nullptr);
    for (std::size_t i = 0; i < keys.size(); ++i) {
        keys[i] = state[KEYMAP[i]] ? 1 : 0;
    }

    return true;
}
