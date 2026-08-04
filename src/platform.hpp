#pragma once

#include <SDL.h>
#include <cstdint>
#include <array>

class Platform {
public:
    Platform(const char* title,
            int windowWidth, int windowHeight,
            int textureWidth, int textureHeight);
    ~Platform();

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    void update(const uint32_t* pixels);
    bool processInput(std::array<uint8_t, 16>& keys);

private:
        void cleanup() noexcept;    // destructors must not throw

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;
        int pitch = 0;
};