#pragma once
#pragma once
#include <Windows.h>
#include <cmath>

namespace offsets {
    constexpr auto localPlayer = 0x18AC00;
    constexpr auto EntityList = 0x18AC04;
    constexpr auto PlayerCount = 0x18AC0C;
    constexpr auto ViewMatrix = 0x17DFD0; // v1.3.0.2

    // Entity Offsets
    constexpr auto health = 0xEC;
    constexpr auto PPosX = 0x28;
    constexpr auto PPosY = 0x2C;
    constexpr auto PPosZ = 0x30;
    constexpr auto PovX = 0x34; // Yaw
    constexpr auto PovY = 0x38; // Pitch
}

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };

// WorldToScreen Math
bool WorldToScreen(Vector3 pos, Vector2& screen, float matrix[16], int width, int height) {
    float x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    float y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    float w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

    if (w < 0.1f) return false;

    float nx = x / w;
    float ny = y / w;

    screen.x = (width / 2.0f * nx) + (nx + width / 2.0f);
    screen.y = -(height / 2.0f * ny) + (ny + height / 2.0f);
    return true;
}