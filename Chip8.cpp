#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <chrono>
#include <random>
#include <limits>
#include <cstdlib>
#include <stdexcept>

#include <SDL2/SDL.h>

constexpr uint16_t START_ADDRESS = 0x200;

constexpr uint8_t V0 = 0x00;
constexpr uint8_t V1 = 0x01;
constexpr uint8_t V2 = 0x02;
constexpr uint8_t V3 = 0x03;
constexpr uint8_t V4 = 0x04;
constexpr uint8_t V5 = 0x05;
constexpr uint8_t V6 = 0x06;
constexpr uint8_t V7 = 0x07;
constexpr uint8_t V8 = 0x08;
constexpr uint8_t V9 = 0x09;
constexpr uint8_t VA = 0x0A;
constexpr uint8_t VB = 0x0B;
constexpr uint8_t VC = 0x0C;
constexpr uint8_t VD = 0x0D;
constexpr uint8_t VE = 0x0E;
constexpr uint8_t VF = 0x0F;

const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START = 0x50;

uint8_t fontset[FONTSET_SIZE] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

class Chip8
{
public:

    Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count())
    {
        PC = START_ADDRESS;
        SP = 0x00;

        for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
            memory[FONTSET_START + i] = fontset[i];
        }

        randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

        table[0x0] = &Chip8::Table0;
        table[0x1] = &Chip8::OP_1NNN;
        table[0x2] = &Chip8::OP_2NNN;
        table[0x3] = &Chip8::OP_3XNN;
        table[0x4] = &Chip8::OP_4XNN;
        table[0x5] = &Chip8::OP_5XY0;
        table[0x6] = &Chip8::OP_6XNN;
        table[0x7] = &Chip8::OP_7XNN;
        table[0x8] = &Chip8::Table8;
        table[0x9] = &Chip8::OP_9XY0;
        table[0xA] = &Chip8::OP_ANNN;
        table[0xB] = &Chip8::OP_BNNN;
        table[0xC] = &Chip8::OP_CXNN;
        table[0xD] = &Chip8::OP_DXYN;
        table[0xE] = &Chip8::TableE;
        table[0xF] = &Chip8::TableF;

        for (size_t i = 0; i <= 0xE; i++)
        {
            table0[i] = &Chip8::OP_NULL;
            table8[i] = &Chip8::OP_NULL;
            tableE[i] = &Chip8::OP_NULL;
        }

        table0[0x0] = &Chip8::OP_00E0;
        table0[0xE] = &Chip8::OP_00EE;

        table8[0x0] = &Chip8::OP_8XY0;
        table8[0x1] = &Chip8::OP_8XY1;
        table8[0x2] = &Chip8::OP_8XY2;
        table8[0x3] = &Chip8::OP_8XY3;
        table8[0x4] = &Chip8::OP_8XY4;
        table8[0x5] = &Chip8::OP_8XY5;
        table8[0x6] = &Chip8::OP_8XY6;
        table8[0x7] = &Chip8::OP_8XY7;
        table8[0xE] = &Chip8::OP_8XYE;

        tableE[0x1] = &Chip8::OP_EXA1;
        tableE[0xE] = &Chip8::OP_EX9E;

        for (size_t i = 0; i <= 0x65; i++)
        {
            tableF[i] = &Chip8::OP_NULL;
        }

        tableF[0x07] = &Chip8::OP_FX07;
        tableF[0x0A] = &Chip8::OP_FX0A;
        tableF[0x15] = &Chip8::OP_FX15;
        tableF[0x18] = &Chip8::OP_FX18;
        tableF[0x1E] = &Chip8::OP_FX1E;
        tableF[0x29] = &Chip8::OP_FX29;
        tableF[0x33] = &Chip8::OP_FX33;
        tableF[0x55] = &Chip8::OP_FX55;
        tableF[0x65] = &Chip8::OP_FX65;

        std::memset(memory, 0, sizeof(memory));
        for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
            memory[FONTSET_START + i] = fontset[i];
        }

        delayTimer = 0;
        soundTimer = 0;
    }

    void loadROM(const char* filename)
    {
        if (filename == nullptr) {
            std::cerr << "Error: ROM filename is null.\n";
            return;
        }

        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            std::cerr << "Error: Unable to open ROM: " << filename << '\n';
            return;
        }

        std::streampos end = file.tellg();

        if (end < 0) {
            std::cerr << "Error: Unable to determine ROM size.\n";
            return;
        }

        const std::streamoff size = static_cast<std::streamoff>(end);
        constexpr size_t MEMORY_SIZE = sizeof(memory);

        if (static_cast<uintmax_t>(size) > MEMORY_SIZE - START_ADDRESS) {
            std::cerr << "Error: ROM exceeds available memory.\n";
            return;
        }

        file.seekg(0, std::ios::beg);

        if (!file) {
            std::cerr << "Error: Unable to seek to ROM start.\n";
            return;
        }

        if (size > 0) {
            file.read(
                reinterpret_cast<char*>(memory + START_ADDRESS),
                static_cast<std::streamsize>(size)
            );

            if (!file || file.gcount() != static_cast<std::streamsize>(size)) {
                std::cerr << "Error: Failed to read the complete ROM.\n";
                return;
            }
        }
    }

    void OP_00E0()
    {
        std::memset(video, 0, sizeof(video));
    }

    void OP_00EE()
    {
        if (SP == 0 || SP > 16) {
            std::cerr << "Error: Stack underflow in 00EE.\n";
            return;
        }

        --SP;
        PC = stack[SP];

        if (PC >= sizeof(memory) - 1) {
            std::cerr << "Error: Return address out of memory range.\n";
            PC = START_ADDRESS;
        }
    }

    void OP_1NNN()
    {
        uint16_t address = opcode & 0x0FFFu;

        if (address >= sizeof(memory)) {
            std::cerr << "Error: Jump address out of range.\n";
            return;
        }

        PC = address;
    }

    void OP_2NNN()
    {
        uint16_t address = opcode & 0x0FFFu;

        if (SP >= 16) {
            std::cerr << "Error: Stack overflow in 2NNN.\n";
            return;
        }

        if (address >= sizeof(memory)) {
            std::cerr << "Error: Call address out of range.\n";
            return;
        }

        stack[SP++] = PC;
        PC = address;
    }

    void OP_3XNN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t value = opcode & 0x00FF;

        if (VX >= 16) return;

        if (registers[VX] == value) {
            PC += 2;
        }
    }

    void OP_4XNN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t value = opcode & 0x00FF;

        if (VX >= 16) return;

        if (registers[VX] != value) {
            PC += 2;
        }
    }

    void OP_5XY0()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        if (registers[VX] == registers[VY]) {
            PC += 2;
        }
    }

    void OP_6XNN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t value = opcode & 0x00FF;

        if (VX >= 16) return;

        registers[VX] = value;
    }

    void OP_7XNN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t value = opcode & 0x00FF;

        if (VX >= 16) return;

        registers[VX] += value;
    }

    void OP_8XY0()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        registers[VX] = registers[VY];
    }

    void OP_8XY1()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        registers[VX] |= registers[VY];
    }

    void OP_8XY2()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        registers[VX] &= registers[VY];
    }

    void OP_8XY3()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        registers[VX] ^= registers[VY];
    }

    void OP_8XY4()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        uint16_t sum = registers[VX] + registers[VY];

        registers[VF] = (sum > 255u) ? 1 : 0;
        registers[VX] = sum & 0xFFu;
    }

    void OP_8XY5()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        uint8_t x = registers[VX];
        uint8_t y = registers[VY];

        registers[VF] = (x >= y) ? 1 : 0;
        registers[VX] = static_cast<uint8_t>(x - y);
    }

    void OP_8XY6()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        registers[VF] = registers[VX] & 0x1u;
        registers[VX] >>= 1;
    }

    void OP_8XY7()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        uint8_t x = registers[VX];
        uint8_t y = registers[VY];

        registers[VF] = (y >= x) ? 1 : 0;
        registers[VX] = static_cast<uint8_t>(y - x);
    }

    void OP_8XYE()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        registers[VF] = (registers[VX] & 0x80u) >> 7u;
        registers[VX] <<= 1;
    }

    void OP_9XY0()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (VX >= 16 || VY >= 16) return;

        if (registers[VX] != registers[VY]) {
            PC += 2;
        }
    }

    void OP_ANNN()
    {
        uint16_t address = opcode & 0x0FFFu;

        if (address >= sizeof(memory)) {
            std::cerr << "Error: Index address out of range.\n";
            return;
        }

        index = address;
    }

    void OP_BNNN()
    {
        uint16_t address = opcode & 0x0FFFu;
        uint32_t target = address + registers[V0];

        if (target >= sizeof(memory)) {
            std::cerr << "Error: BNNN jump target out of range.\n";
            return;
        }

        PC = static_cast<uint16_t>(target);
    }

    void OP_CXNN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        if (VX >= 16) return;

        registers[VX] = randByte(randGen) & byte;
    }

    void OP_DXYN()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;
        uint8_t height = opcode & 0x000Fu;

        if (VX >= 16 || VY >= 16) return;

        if (height == 0) {
            registers[VF] = 0;
            return;
        }

        if (index >= sizeof(memory) ||
            height > sizeof(memory) - index) {
            std::cerr << "Error: Sprite data exceeds memory bounds.\n";
            return;
        }

        uint8_t xPos = registers[VX] % VIDEO_WIDTH;
        uint8_t yPos = registers[VY] % VIDEO_HEIGHT;

        registers[VF] = 0;

        for (unsigned int row = 0; row < height; ++row) {
            uint8_t spriteByte = memory[index + row];

            for (unsigned int col = 0; col < 8; ++col) {
                uint8_t spritePixel = spriteByte & (0x80u >> col);

                if (!spritePixel) continue;

                unsigned int x = (xPos + col) % VIDEO_WIDTH;
                unsigned int y = (yPos + row) % VIDEO_HEIGHT;
                size_t pixelIndex = y * VIDEO_WIDTH + x;

                if (pixelIndex >= VIDEO_WIDTH * VIDEO_HEIGHT) {
                    continue;
                }

                uint32_t* screenPixel = &video[pixelIndex];

                if (*screenPixel == 0xFFFFFFFFu) {
                    registers[VF] = 1;
                }

                *screenPixel ^= 0xFFFFFFFFu;
            }
        }
    }

    void OP_EX9E()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        uint8_t key = registers[VX];

        if (key >= 16) {
            std::cerr << "Error: Keypad index out of range.\n";
            return;
        }

        if (keypad[key]) {
            PC += 2;
        }
    }

    void OP_EXA1()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        uint8_t key = registers[VX];

        if (key >= 16) {
            std::cerr << "Error: Keypad index out of range.\n";
            return;
        }

        if (!keypad[key]) {
            PC += 2;
        }
    }

    void OP_FX07()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        registers[VX] = delayTimer;
    }

    void OP_FX0A()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        for (uint8_t key = 0; key < 16; ++key) {
            if (keypad[key]) {
                registers[VX] = key;
                return;
            }
        }

        if (PC >= 2) {
            PC -= 2;
        }
        else {
            std::cerr << "Error: PC underflow while waiting for key.\n";
        }
    }

    void OP_FX15()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        delayTimer = registers[VX];
    }

    void OP_FX18()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        soundTimer = registers[VX];
    }

    void OP_FX1E()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        uint32_t result = index + registers[VX];

        if (result >= sizeof(memory)) {
            std::cerr << "Error: Index register overflow.\n";
            return;
        }

        index = static_cast<uint16_t>(result);
    }

    void OP_FX29()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        uint8_t digit = registers[VX] & 0x0Fu;
        uint16_t address = FONTSET_START + 5u * digit;

        if (address + 4 >= sizeof(memory)) {
            std::cerr << "Error: Font address out of range.\n";
            return;
        }

        index = address;
    }

    void OP_FX33()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        if (index >= sizeof(memory) || sizeof(memory) - index < 3) {
            std::cerr << "Error: BCD storage exceeds memory bounds.\n";
            return;
        }

        uint8_t value = registers[VX];

        memory[index + 2] = value % 10;
        value /= 10;

        memory[index + 1] = value % 10;
        value /= 10;

        memory[index] = value % 10;
    }

    void OP_FX55()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        if (index >= sizeof(memory) ||
            static_cast<size_t>(VX) + 1 > sizeof(memory) - index) {
            std::cerr << "Error: Register storage exceeds memory bounds.\n";
            return;
        }

        for (uint8_t i = 0; i <= VX; ++i) {
            memory[index + i] = registers[i];
        }
    }

    void OP_FX65()
    {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (VX >= 16) return;

        if (index >= sizeof(memory) ||
            static_cast<size_t>(VX) + 1 > sizeof(memory) - index) {
            std::cerr << "Error: Register loading exceeds memory bounds.\n";
            return;
        }

        for (uint8_t i = 0; i <= VX; ++i) {
            registers[i] = memory[index + i];
        }
    }

    void Table0()
    {
        size_t subOpcode = opcode & 0x000Fu;

        if (subOpcode >= sizeof(table0) / sizeof(table0[0])) {
            std::cerr << "Error: Invalid Table0 opcode.\n";
            return;
        }

        ((*this).*(table0[subOpcode]))();
    }

    void Table8()
    {
        size_t subOpcode = opcode & 0x000Fu;

        if (subOpcode >= sizeof(table8) / sizeof(table8[0])) {
            std::cerr << "Error: Invalid Table8 opcode.\n";
            return;
        }

        ((*this).*(table8[subOpcode]))();
    }

    void TableE()
    {
        size_t subOpcode = opcode & 0x000Fu;

        if (subOpcode >= sizeof(tableE) / sizeof(tableE[0])) {
            std::cerr << "Error: Invalid TableE opcode.\n";
            return;
        }

        ((*this).*(tableE[subOpcode]))();
    }

    void TableF()
    {
        size_t subOpcode = opcode & 0x00FFu;

        if (subOpcode >= sizeof(tableF) / sizeof(tableF[0])) {
            std::cerr << "Error: Invalid TableF opcode.\n";
            return;
        }

        ((*this).*(tableF[subOpcode]))();
    }

    void OP_NULL()
    {}

    void Cycle()
    {
        if (PC >= sizeof(memory) || sizeof(memory) - PC < 2) {
            std::cerr << "Error: Program counter out of memory bounds.\n";
            PC = START_ADDRESS;
            return;
        }

        opcode = (static_cast<uint16_t>(memory[PC]) << 8u) |
                 static_cast<uint16_t>(memory[PC + 1]);

        PC += 2;

        size_t tableIndex = (opcode & 0xF000u) >> 12u;

        if (tableIndex >= sizeof(table) / sizeof(table[0])) {
            std::cerr << "Error: Invalid opcode table index.\n";
            return;
        }

        ((*this).*(table[tableIndex]))();

        if (delayTimer > 0) --delayTimer;
        if (soundTimer > 0) --soundTimer;
    }

    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t PC{};
    uint8_t SP{};
    uint16_t stack[16]{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode{};

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    typedef void (Chip8::*Chip8Func)();

    Chip8Func table[0xF + 1]{};
    Chip8Func table0[0xE + 1]{};
    Chip8Func table8[0xE + 1]{};
    Chip8Func tableE[0xE + 1]{};
    Chip8Func tableF[0x65 + 1]{};
};

class Platform
{
public:
    Platform(char const* title, int windowWidth, int windowHeight,
             int textureWidth, int textureHeight)
    {
        if (windowWidth <= 0 || windowHeight <= 0 ||
            textureWidth <= 0 || textureHeight <= 0) {
            throw std::invalid_argument("Window and texture dimensions must be positive.");
        }

        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(
                std::string("SDL_Init failed: ") + SDL_GetError()
            );
        }

        window = SDL_CreateWindow(
            title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN
        );

        if (!window) {
            SDL_Quit();
            throw std::runtime_error(
                std::string("SDL_CreateWindow failed: ") + SDL_GetError()
            );
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        if (!renderer) {
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            throw std::runtime_error(
                std::string("SDL_CreateRenderer failed: ") + SDL_GetError()
            );
        }

        texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            textureWidth,
            textureHeight
        );

        if (!texture) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            throw std::runtime_error(
                std::string("SDL_CreateTexture failed: ") + SDL_GetError()
            );
        }
    }

    ~Platform()
    {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);

        SDL_Quit();
    }

    void Update(void const* buffer, int pitch)
    {
        if (!buffer || pitch <= 0 || !texture || !renderer) {
            std::cerr << "Error: Invalid rendering buffer or pitch.\n";
            return;
        }

        if (SDL_UpdateTexture(texture, nullptr, buffer, pitch) != 0) {
            std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << '\n';
            return;
        }

        if (SDL_RenderClear(renderer) != 0) {
            std::cerr << "SDL_RenderClear failed: " << SDL_GetError() << '\n';
            return;
        }

        if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
            std::cerr << "SDL_RenderCopy failed: " << SDL_GetError() << '\n';
            return;
        }

        SDL_RenderPresent(renderer);
    }

    bool ProcessInput(uint8_t* keys)
    {
        if (!keys) {
            std::cerr << "Error: Keypad pointer is null.\n";
            return true;
        }

        bool quit = false;
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    bool pressed = (event.type == SDL_KEYDOWN);

                    if (event.type == SDL_KEYDOWN && event.key.repeat) {
                        break;
                    }

                    int keyIndex = -1;

                    switch (event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            if (pressed) quit = true;
                            break;

                        case SDLK_x: keyIndex = 0x0; break;
                        case SDLK_1: keyIndex = 0x1; break;
                        case SDLK_2: keyIndex = 0x2; break;
                        case SDLK_3: keyIndex = 0x3; break;
                        case SDLK_q: keyIndex = 0x4; break;
                        case SDLK_w: keyIndex = 0x5; break;
                        case SDLK_e: keyIndex = 0x6; break;
                        case SDLK_a: keyIndex = 0x7; break;
                        case SDLK_s: keyIndex = 0x8; break;
                        case SDLK_d: keyIndex = 0x9; break;
                        case SDLK_z: keyIndex = 0xA; break;
                        case SDLK_c: keyIndex = 0xB; break;
                        case SDLK_4: keyIndex = 0xC; break;
                        case SDLK_r: keyIndex = 0xD; break;
                        case SDLK_f: keyIndex = 0xE; break;
                        case SDLK_v: keyIndex = 0xF; break;

                        default:
                            break;
                    }

                    if (keyIndex >= 0 &&
                        static_cast<size_t>(keyIndex) < 16) {
                        keys[keyIndex] = pressed ? 1 : 0;
                    }
                }
                break;

                default:
                    break;
            }
        }

        return quit;
    }

private:
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
};

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
        return EXIT_FAILURE;
    }

    int scale_factor = 0;
    int cycle_delay = 0;

    try {
        size_t parsed = 0;

        scale_factor = std::stoi(argv[1], &parsed);

        if (argv[1][parsed] != '\0' || scale_factor <= 0) {
            std::cerr << "Error: Scale must be a positive integer.\n";
            return EXIT_FAILURE;
        }

        parsed = 0;
        cycle_delay = std::stoi(argv[2], &parsed);

        if (argv[2][parsed] != '\0' || cycle_delay <= 0) {
            std::cerr << "Error: Delay must be a positive integer.\n";
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: Invalid command-line number: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    if (argv[3] == nullptr || argv[3][0] == '\0') {
        std::cerr << "Error: ROM filename cannot be empty.\n";
        return EXIT_FAILURE;
    }

    if (scale_factor > std::numeric_limits<int>::max() /
                       static_cast<int>(VIDEO_WIDTH) ||
        scale_factor > std::numeric_limits<int>::max() /
                       static_cast<int>(VIDEO_HEIGHT)) {
        std::cerr << "Error: Scale factor is too large.\n";
        return EXIT_FAILURE;
    }

    const char* romFilename = argv[3];

    try {
        Platform platform(
            "CHIP-8 Emulator",
            VIDEO_WIDTH * scale_factor,
            VIDEO_HEIGHT * scale_factor,
            VIDEO_WIDTH,
            VIDEO_HEIGHT
        );

        Chip8 chip8;
        chip8.loadROM(romFilename);

        int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

        auto lastCycleTime = std::chrono::high_resolution_clock::now();
        bool quit = false;

        while (!quit)
        {
            quit = platform.ProcessInput(chip8.keypad);

            auto currentTime = std::chrono::high_resolution_clock::now();

            float dt = std::chrono::duration<float,
                std::chrono::milliseconds::period>(
                    currentTime - lastCycleTime
                ).count();

            if (dt > cycle_delay)
            {
                lastCycleTime = currentTime;

                chip8.Cycle();

                platform.Update(chip8.video, videoPitch);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
