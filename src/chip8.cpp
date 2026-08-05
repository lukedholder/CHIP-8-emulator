#include "chip8.hpp"

#include <fstream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cstddef>


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

Chip8::Chip8(Quirks q) : quirks(q), rng(std::random_device{}()) {
    pc = START_ADDRESS;

    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
        writeMemory(static_cast<uint16_t>(FONTSET_START_ADDRESS + i), FONTSET[i]);
    }
}

bool Chip8::loadROM(const uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0 || size > MEMORY_SIZE - START_ADDRESS) {
        return false;
    }
    std::copy_n(data, size, memory.begin() + START_ADDRESS);
    return true;
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
    if (size <= 0) {
        return false;
    }

    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    return loadROM(buffer.data(), buffer.size());
}

void Chip8::unknownOpcode(uint16_t opcode) {
    const uint16_t addr = static_cast<uint16_t>(pc - 2);
    if (reportedUnknown[addr]) {
        return;
    }
    reportedUnknown[addr] = true;
    std::fprintf(stderr, "Unknown opcode %04X at %03X\n", opcode, addr);
}

void Chip8::tickTimers() {
    if (delayTimer > 0) {
        --delayTimer;
    }
    if (soundTimer > 0) {
        --soundTimer;
    }
}

void Chip8::cycle() {
    const uint16_t opcode = static_cast<uint16_t>(
        (readMemory(pc) << 8) | readMemory(static_cast<uint16_t>(pc + 1)));
    pc += 2;

    const uint8_t x     = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t y     = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
    const uint8_t kk    = static_cast<uint8_t>(opcode & 0x00FF);
    const uint16_t nnn  = static_cast<uint16_t>(opcode & 0x0FFF);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:    // 00E0: clear the display
                    video.fill(0);  // std::array::fill is one call instead of a loop
                    break;

                case 0x00EE:    // 00EE: return from subroutine
                    if (sp == 0) {
                        std::fprintf(stderr, "Stack underflow at %03X\n", pc - 2);
                        break;
                    }
                    --sp;
                    pc = stack[sp];
                    break;

                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;
        
        case 0x1000:    // 1NNN: jump to NNN
            pc = nnn;
            break;

        case 0x2000:    // 2NNN: call subroutine at NNN
            if (sp >= STACK_SIZE) {
                std::fprintf(stderr, "Stack overflow at %03X\n", pc - 2);
                break;
            }
            stack[sp] = pc;
            ++sp;
            pc = nnn;
            break;

        case 0x3000:    // 3XNN: skip next if VX == NN
            if (V[x] == kk) {
                pc += 2;
            }
            break;

        case 0x4000:    // 4XNN: skip next if VX != NN
            if (V[x] != kk) {
                pc += 2;
            }
            break;

        case 0x5000:    // 5XY0: skip next if VX == VY
            if ((opcode & 0x000F) != 0) {
                unknownOpcode(opcode);
                break;
            }
            if (V[x] == V[y]) {
                pc += 2;
            }
            break;

        case 0x6000:    // 6XNN: VX = NN
            V[x] = kk;
            break;

        case 0x7000:    // 7XNN: VX += NN (no carry)
            V[x] = static_cast<uint8_t>(V[x] + kk);
            break;

        case 0x8000:    // Arithmetic family
            switch (opcode & 0x000F) {
                case 0x0000:    // 8XY0: VX = VY
                    V[x] = V[y];
                    break;
                
                case 0x0001:    // 8XY1: VX |= VY   (OR)
                    V[x] = static_cast<uint8_t>(V[x] | V[y]);
                    if (quirks.logicResetsVF) {
                        V[0xF] = 0;
                    }
                    break;

                case 0x0002:    // 8XY2: VX &= VY   (AND)
                    V[x] = static_cast<uint8_t>(V[x] & V[y]);
                    if (quirks.logicResetsVF) {
                        V[0xF] = 0;
                    }
                    break;

                case 0x0003:    // 8XY3: VX ^= VY   (XOR)
                    V[x] = static_cast<uint8_t>(V[x] ^ V[y]);
                    if (quirks.logicResetsVF) {
                        V[0xF] = 0;
                    }
                    break;

                case 0x0004: {  // 8XY4: VX += VY, VF = carry
                    const unsigned int sum = V[x] + V[y];
                    V[x] = static_cast<uint8_t>(sum);
                    V[0xF] = (sum > 0xFF) ? 1 : 0;
                    break;
                }

                case 0x0005: {  // 8XY5: VX -= VY, VF = 1 if no borrow
                    const uint8_t flag = (V[x] >= V[y]) ? 1 : 0;
                    V[x] = static_cast<uint8_t>(V[x] - V[y]);
                    V[0xF] = flag;
                    break;
                }

                case 0x0006: {  // 8XY6: VX >>= 1, VF = bit shifted out
                    const uint8_t source = quirks.shiftUsesVY ? V[y] : V[x];
                    const uint8_t flag = static_cast<uint8_t>(source & 0x01);
                    V[x] = static_cast<uint8_t>(source >> 1);
                    V[0xF] = flag;
                    break;
                }

                case 0x0007: {  // 8XY7: VX = VY - VX, VF = 1 if no borrow
                    const uint8_t flag = (V[y] >= V[x]) ? 1 : 0;
                    V[x] =  static_cast<uint8_t>(V[y] - V[x]);
                    V[0xF] = flag;
                    break;
                }

                case 0x000E: {  // 8XYE: VX <<= 1, VF = bit shifted out
                    const uint8_t flag = static_cast<uint8_t>((V[x] & 0x80) >> 7);
                    V[x] =  static_cast<uint8_t>(V[x] << 1);
                    V[0xF] = flag;
                    break;  // note: VY was intentionally unused (modern quirk)
                }

                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;

        case 0x9000: // 9XY0: skip next if VX != VY
            if ((opcode & 0x000F) != 0) {
                unknownOpcode(opcode);
                break;
            }
            if (V[x] != V[y]) {
                pc += 2;
            }
            break;

        case 0xA000:    // ANNN: I = NNN
            I = nnn;
            break;

        case 0xB000: {  // BNNN / BXNN: jump with register offset
            const uint8_t offset = quirks.jumpUsesVX ? V[x] : V[0];
            pc = static_cast<uint16_t>((nnn + offset) & 0x0FFF);
            break;
        }

        case 0xC000:    // CXNN: VX = random byte AND NN
            V[x] = static_cast<uint8_t>(randomByte(rng) & kk);
            break;

        case 0xD000: {  // DXYN: draw N-row sprite from I at (VX, VY)
            const uint8_t n = static_cast<uint8_t>((opcode & 0x000F));

            const unsigned int startX = V[x] % VIDEO_WIDTH;
            const unsigned int startY = V[y] % VIDEO_HEIGHT;

            V[0xF] = 0; // reset collision flag at register[15]

            for (unsigned int row = 0; row < n; ++row) {
                const unsigned int py = startY + row;
                if (py >= VIDEO_HEIGHT) {
                    break;  // clip at the bottom edge
                }

                const uint8_t spriteByte = readMemory(static_cast<uint16_t>(I + row));

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

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E:    //EX9E: skip next if key VX is pressed
                    if (keypad[V[x] & 0x0F] != 0) {
                        pc += 2;
                    }
                    break;

                case 0x00A1:    // EXA1: skip next if VX is NOT pressed
                    if (keypad[V[x] & 0x0F] == 0) {
                        pc +=2;
                    }
                    break;

                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007:    // FX07: VX = delay timer
                    V[x] = delayTimer;
                    break;

                case 0x0015:    // FX15: delay timer = VX
                    delayTimer = V[x];
                    break;

                case 0x0018:    // FX18: sound timer = VX
                    soundTimer = V[x];
                    break;

                case 0x001E:    // FX1E: I += VX
                    I = static_cast<uint16_t>(I + V[x]);
                    break;

                case 0x0029:    // FX29: I = address of font sprite for digit VX
                    I = static_cast<uint16_t> (FONTSET_START_ADDRESS + (V[x] & 0x0F) * 5);
                    break;

                case 0x0033: {  // FX33: BCD of VX into memory[I...I+2]
                    const uint8_t value = V[x];
                    writeMemory(I, static_cast<uint8_t>(value / 100));
                    writeMemory(I + 1, static_cast<uint8_t>(value / 10) % 10);
                    writeMemory(I + 2, static_cast<uint8_t>(value % 10));
                    break;
                }

                case 0x0055:    // FX55: store V0...VX to memory at I
                    for (uint8_t i = 0; i <= x; ++i) {
                        writeMemory(static_cast<uint16_t>(I + i), V[i]);
                    }
                    if (quirks.loadStoreIncrementsI) {
                        I = static_cast<uint16_t>(I + x + 1);
                    }
                    break;

                case 0x0065:    // FX65: load V0...VX from memory at I
                    for (uint8_t i = 0; i <= x; ++i) {
                        V[i] = readMemory(I + i);
                    }
                    break;
                
                case 0x000A: {  // FX0A: wait for a key press, store it in VX
                    bool pressed = false;
                    for (uint8_t k = 0; k < KEY_COUNT; ++k) {
                        if (keypad[k] != 0) {
                            V[x] = k;
                            pressed = true;
                            break;
                        }
                    }
                    if (!pressed) {
                        pc -= 2;    // re-run this instructino next cycle
                    }
                    break;
                }

                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;

        default:
            unknownOpcode(opcode);
            break;
    }
}