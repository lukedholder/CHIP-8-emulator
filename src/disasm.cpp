#include "disasm.hpp"

#include <cstdio>

namespace {

std::string format(const char* fmt, ...) = delete;  // avoid varargs; see below

}  // namespace

std::string disassemble(uint16_t opcode) {
    char buf[24];

    const uint8_t  x   = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
    const uint8_t  y   = static_cast<uint8_t>((opcode & 0x00F0) >> 4);
    const uint8_t  n   = static_cast<uint8_t>(opcode & 0x000F);
    const uint8_t  kk  = static_cast<uint8_t>(opcode & 0x00FF);
    const uint16_t nnn = static_cast<uint16_t>(opcode & 0x0FFF);

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) { return "CLS"; }
            if (opcode == 0x00EE) { return "RET"; }
            break;

        case 0x1000: std::snprintf(buf, sizeof buf, "JP    %03X", nnn); return buf;
        case 0x2000: std::snprintf(buf, sizeof buf, "CALL  %03X", nnn); return buf;
        case 0x3000: std::snprintf(buf, sizeof buf, "SE    V%X, %02X", x, kk); return buf;
        case 0x4000: std::snprintf(buf, sizeof buf, "SNE   V%X, %02X", x, kk); return buf;
        case 0x5000: std::snprintf(buf, sizeof buf, "SE    V%X, V%X", x, y); return buf;
        case 0x6000: std::snprintf(buf, sizeof buf, "LD    V%X, %02X", x, kk); return buf;
        case 0x7000: std::snprintf(buf, sizeof buf, "ADD   V%X, %02X", x, kk); return buf;

        case 0x8000:
            switch (n) {
                case 0x0: std::snprintf(buf, sizeof buf, "LD    V%X, V%X", x, y); return buf;
                case 0x1: std::snprintf(buf, sizeof buf, "OR    V%X, V%X", x, y); return buf;
                case 0x2: std::snprintf(buf, sizeof buf, "AND   V%X, V%X", x, y); return buf;
                case 0x3: std::snprintf(buf, sizeof buf, "XOR   V%X, V%X", x, y); return buf;
                case 0x4: std::snprintf(buf, sizeof buf, "ADD   V%X, V%X", x, y); return buf;
                case 0x5: std::snprintf(buf, sizeof buf, "SUB   V%X, V%X", x, y); return buf;
                case 0x6: std::snprintf(buf, sizeof buf, "SHR   V%X", x); return buf;
                case 0x7: std::snprintf(buf, sizeof buf, "SUBN  V%X, V%X", x, y); return buf;
                case 0xE: std::snprintf(buf, sizeof buf, "SHL   V%X", x); return buf;
                default: break;
            }
            break;

        case 0x9000: std::snprintf(buf, sizeof buf, "SNE   V%X, V%X", x, y); return buf;
        case 0xA000: std::snprintf(buf, sizeof buf, "LD    I, %03X", nnn); return buf;
        case 0xB000: std::snprintf(buf, sizeof buf, "JP    V0, %03X", nnn); return buf;
        case 0xC000: std::snprintf(buf, sizeof buf, "RND   V%X, %02X", x, kk); return buf;
        case 0xD000: std::snprintf(buf, sizeof buf, "DRW   V%X, V%X, %X", x, y, n); return buf;

        case 0xE000:
            if (kk == 0x9E) { std::snprintf(buf, sizeof buf, "SKP   V%X", x); return buf; }
            if (kk == 0xA1) { std::snprintf(buf, sizeof buf, "SKNP  V%X", x); return buf; }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: std::snprintf(buf, sizeof buf, "LD    V%X, DT", x); return buf;
                case 0x0A: std::snprintf(buf, sizeof buf, "LD    V%X, K", x); return buf;
                case 0x15: std::snprintf(buf, sizeof buf, "LD    DT, V%X", x); return buf;
                case 0x18: std::snprintf(buf, sizeof buf, "LD    ST, V%X", x); return buf;
                case 0x1E: std::snprintf(buf, sizeof buf, "ADD   I, V%X", x); return buf;
                case 0x29: std::snprintf(buf, sizeof buf, "LD    F, V%X", x); return buf;
                case 0x33: std::snprintf(buf, sizeof buf, "LD    B, V%X", x); return buf;
                case 0x55: std::snprintf(buf, sizeof buf, "LD    [I], V%X", x); return buf;
                case 0x65: std::snprintf(buf, sizeof buf, "LD    V%X, [I]", x); return buf;
                default: break;
            }
            break;

        default: break;
    }

    std::snprintf(buf, sizeof buf, "DW    %04X", opcode);
    return buf;
}