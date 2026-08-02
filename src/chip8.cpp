#include "chip8.hpp"

#include <fstream>
#include <cstdio>


namespace {

constexpr std::array<uint8_t, Chip8::FONTSET_SIZE> FONTSET = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

}

Chip8::Chip8() {
    pc = START_ADDRESS;

    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        memory[FONTSET_START_ADDRESS + i] = FONTSET[i];
    }
}

bool Chip8::loadROM(const std::string& filename) {
    // Always open binary files in binary mode to avoid merging the two '\r\n' bytes into one '\n' byte on read (Windows)
    // a ROM is machine code, not text -- any 0x0D 0x0A in it would be corrupted into 0x0A
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // 'std::ios::ate' means "seek to the end on open" so tellg() immediately gives you the file size.
    if (!file) {
        return false;
    }

    const std::streamsize size = file.tellg();
    // Reject oversized ROMs: writing past memory.end() would be a buffer overflow.
    if (size <= 0 || static_cast<unsigned int>(size) > MEMORY_SIZE - START_ADDRESS) {
        return false;
    }

    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(memory.data() + START_ADDRESS), size)) {
        return false;
    }

    return true;
}

void Chip8::unknownOpcode(uint16_t opcode) const {
    const uint16_t addr = static_cast<uint16_t>(pc - 2);
    if (reportedUnknown[addr]) {
        return;
    }
    reportedUnknown[addr] = true;
    std::fprintf(stderr, "Unknown opcode %04X at %03X\n", opcode, addr);
}

void Chip8::cycle() {
    const uint16_t opcode = static_cast<uint16_t>((memory[pc] << 8) | memory[pc + 1]);
    pc += 2;

    const uint8_t x     = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t kk    = static_cast<uint8_t>(opcode & 0x00FF);
    const uint16_t nnn  = static_cast<uint16_t>(opcode & 0x0FFF);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:    // 00E0: clear the display
                    video.fill(0);  // std::array::fill is one call instead of a loop
                    break;

                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;
        
        case 0x1000:    // 1NNN: jump to NNN
            pc = nnn;
            break;

        case 0x6000:    // 6XNN: VX = NN
            V[x] = kk;
            break;

        case 0x7000:    // 7XNN: VX += NN (no carry)
            V[x] = static_cast<uint8_t>(V[x] + kk);
            break;

        case 0xA000:    // ANNN: I = NNN
            I = nnn;
            break;

        case 0xD000: {  // DXYN: draw N-row sprite from I at (VX, VY)
            const uint8_t y = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
            const uint8_t n = static_cast<uint8_t>((opcode & 0x000F));

            const unsigned int startX = V[x] % VIDEO_WIDTH;
            const unsigned int startY = V[y] % VIDEO_HEIGHT;

            V[0xF] = 0; // reset collision flag at register[15]

            for (unsigned int row = 0; row < n; ++row) {
                const unsigned int py = startY + row;
                if (py >= VIDEO_HEIGHT) {
                    break;  // clip at the bottom edge
                }

                const uint8_t spriteByte = memory[I + row];

                for (unsigned int col = 0; col < 8; ++col) {
                    const unsigned int px = startX + col;
                    if (px >= VIDEO_WIDTH) {
                        break;  // clip at the right edge
                    }

                    if ((spriteByte & (0x80u >> col)) == 0) {
                        continue;   // sprite bit clear: leave the screen
                    }

                    uint32_t& screenPixel = video[py * VIDEO_WIDTH + px];

                    if (screenPixel != 0) {
                        V[0xF] = 1; // a lit pixel is about to be turned off
                    }

                    screenPixel ^= 0xFFFFFFFFu;
                }
            }

            break;
        }

        default:
            unknownOpcode(opcode);
            break;
    }
}