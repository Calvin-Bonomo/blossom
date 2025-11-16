#pragma once

#include <cstdint>

struct Settings {
    uint32_t width, height;

    Settings(): width(600), height(800) { }

    Settings(uint32_t _width, uint32_t _height): width(_width), height(_height) { }
};
