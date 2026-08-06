#include "platform.hpp"

#include <stdexcept>
#include <string>

Platform::Platform(const char* title,
                    int windowWidth, int windowHeight,
                    int textureWidth, int textureHeight) {
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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

    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = &Platform::audioCallback;
    want.userdata = this;

    SDL_AudioSpec have{};
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, & have, 0);
    if (audioDevice != 0) {
        phaseIncrement = 440.0 / have.freq; // A4
    }
}

Platform::~Platform() {
    cleanup();
}

void Platform::cleanup() noexcept {
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
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

InputResult Platform::processInput(std::array<uint8_t, 16>& keys) {
    InputResult result;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            result.quit = true;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:   result.quit = true; break;
                case SDLK_F1:       result.togglePause = true; break;
                case SDLK_F2:       result.step = true; break;
                case SDLK_F3:       result.dumpState = true; break;
                default: break;
            }
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

    return result;
}

void Platform::audioCallback(void* userdata, Uint8* stream, int len) {
    Platform* self = static_cast<Platform*>(userdata);

    int16_t* out = reinterpret_cast<int16_t*>(stream);
    const int sampleCount = len / static_cast<int>(sizeof(int16_t));

    constexpr int16_t AMPLITUDE = 3000;
    
    for (int i = 0; i < sampleCount; ++i) {
        out[i] = (self->phase < 0.5) ? AMPLITUDE : static_cast<int16_t>(-AMPLITUDE);

        self->phase += self->phaseIncrement;
        if (self->phase >= 1.0) {
            self->phase -= 1.0;
        }
    }
}

void Platform::setBeep(bool on) {
    if (audioDevice == 0 || on == beeping) {
        return;
    }
    beeping = on;
    SDL_PauseAudioDevice(audioDevice, on ? 0 : 1);
}