#include <catch2/catch_test_macros.hpp>

#include "chip8.hpp"
#include "disasm.hpp"

#include <vector>

namespace {

// Build a machine loaded with a program at 0x200
Chip8 machineWith(std::vector<uint8_t> program, Quirks q = Quirks::chip8()) {
    Chip8 chip8(q);
    chip8.loadROM(program.data(), program.size());
    return chip8;
}

} // namespace

TEST_CASE("FX33 splits a byte into decimal digits") {
    // 6A89 = V10 = 137 ; A400 = I = 0x400 ; FA33 = BCD of V10
    auto chip8 = machineWith({0x6A, 0x89, 0xA4, 0x00, 0xFA, 0x33});
    chip8.cycle();
    chip8.cycle();
    chip8.cycle();

    CHECK(chip8.peek(0x400) == 1);
    CHECK(chip8.peek(0x401) == 3);
    CHECK(chip8.peek(0x402) == 7);
}

TEST_CASE("8XY4 wraps and sets the carry flag") {
    // V0 = 200 ; V1 = 100 ; V0 += V1
    auto chip8 = machineWith({0x60, 0xC8, 0x61, 0x64, 0x80, 0x14});
    chip8.cycle();
    chip8.cycle();
    chip8.cycle();

    CHECK(chip8.reg(0) == 44);
    CHECK(chip8.reg(0xF) == 1);
}

TEST_CASE("8XY5 clears VF on borrow and sets it otherwise") {
    SECTION("borrow") {
        auto chip8 = machineWith({0x60, 0x0A, 0x61, 0x14, 0x80, 0x15});
        chip8.cycle();
        chip8.cycle();
        chip8.cycle();
        CHECK(chip8.reg(0) == 246);   // 10 - 20 wraps
        CHECK(chip8.reg(0xF) == 0);
    }
    SECTION("no borrow") {
        auto chip8 = machineWith({0x60, 0x14, 0x61, 0x0A, 0x80, 0x15});
        chip8.cycle();
        chip8.cycle();
        chip8.cycle();
        CHECK(chip8.reg(0) == 10);
        CHECK(chip8.reg(0xF) == 1);
    }
}

TEST_CASE("the flag write wins when VF is the destination") {
    // VF = 200 ; V1 = 100 ; VF += V1  → VF holds the carry, not the sum
    auto chip8 = machineWith({0x6F, 0xC8, 0x61, 0x64, 0x8F, 0x14});
    chip8.cycle();
    chip8.cycle();
    chip8.cycle();

    CHECK(chip8.reg(0xF) == 1);
}

TEST_CASE("FX29 points I at the font glyph for a digit") {
    auto chip8 = machineWith({0x60, 0x07, 0xF0, 0x29});
    chip8.cycle();
    chip8.cycle();

    CHECK(chip8.indexRegister() == Chip8::FONTSET_START_ADDRESS + 7 * 5);
}

TEST_CASE("2NNN and 00EE round-trip through the stack") {
    // 2204 = call 0x204 ; (0x202 unused) ; 00EE = return
    auto chip8 = machineWith({0x22, 0x04, 0x00, 0x00, 0x00, 0xEE});

    chip8.cycle();
    CHECK(chip8.programCounter() == 0x204);
    CHECK(chip8.stackPointer() == 1);

    chip8.cycle();
    CHECK(chip8.programCounter() == 0x202);
    CHECK(chip8.stackPointer() == 0);
}

TEST_CASE("FX55 stores an inclusive register range") {
    // V0=1 V1=2 V2=3 ; I=0x400 ; F255 stores V0..V2
    auto chip8 = machineWith({0x60, 0x01, 0x61, 0x02, 0x62, 0x03,
                              0xA4, 0x00, 0xF2, 0x55});
    for (int i = 0; i < 5; ++i) {
        chip8.cycle();
    }

    CHECK(chip8.peek(0x400) == 1);
    CHECK(chip8.peek(0x401) == 2);
    CHECK(chip8.peek(0x402) == 3);   // inclusive — V2 must be stored
}

TEST_CASE("the load/store quirk controls whether I advances") {
    const std::vector<uint8_t> program{0xA4, 0x00, 0xF2, 0x55};

    SECTION("COSMAC: I advances by X+1") {
        auto chip8 = machineWith(program, Quirks::chip8());
        chip8.cycle();
        chip8.cycle();
        CHECK(chip8.indexRegister() == 0x403);
    }
    SECTION("SUPER-CHIP: I is unchanged") {
        auto chip8 = machineWith(program, Quirks::superChip());
        chip8.cycle();
        chip8.cycle();
        CHECK(chip8.indexRegister() == 0x400);
    }
}

TEST_CASE("disassembler produces standard mnemonics") {
    CHECK(disassemble(0x00E0) == "CLS");
    CHECK(disassemble(0xA22A) == "LD    I, 22A");
    CHECK(disassemble(0xD01F) == "DRW   V0, V1, F");
    CHECK(disassemble(0x8124) == "ADD   V1, V2");
    CHECK(disassemble(0xFFFF) == "DW    FFFF");
}