#include <SDL.h>
#include <cstdio>
#include "chip8.hpp"

constexpr int VIDEO_WIDTH = 64;
constexpr int VIDEO_HEIGHT = 32;
constexpr int SCALE = 15;

int main(int argc, char* argv[]) {
    // Take a ROM path from the command line
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <rom-file>\n", argv[0]);
        return 1;
    }

    Chip8 chip8;
    if (!chip8.loadROM(argv[1])) {
        std::fprintf(stderr, "Failed to load ROM: %s\n", argv[1]);
        return 1;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        VIDEO_WIDTH * SCALE,
        VIDEO_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}