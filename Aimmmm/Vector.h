#pragma once
#include <cmath>
#include <random>
#include <algorithm>

constexpr float PI = 3.14159265358979323846f;
constexpr float M_PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    float Distance(const Vec2& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct Vector4 {
    float x, y, z, w; // w is the alpha channel
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float Distance(const Vec3& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Vec3 Normalize() const {
        float len = Length();
        if (len == 0) return Vec3();
        return Vec3(x / len, y / len, z / len);
    }

    float Dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
};

struct ViewMatrix {
    float matrix[16];
};

inline Vec2 WorldToScreen(const ViewMatrix& viewMatrix, const Vec3& worldPos, int screenWidth, int screenHeight) {
    Vec3 screenPos;

    screenPos.x = viewMatrix.matrix[0] * worldPos.x + viewMatrix.matrix[1] * worldPos.y + viewMatrix.matrix[2] * worldPos.z + viewMatrix.matrix[3];
    screenPos.y = viewMatrix.matrix[4] * worldPos.x + viewMatrix.matrix[5] * worldPos.y + viewMatrix.matrix[6] * worldPos.z + viewMatrix.matrix[7];
    screenPos.z = viewMatrix.matrix[8] * worldPos.x + viewMatrix.matrix[9] * worldPos.y + viewMatrix.matrix[10] * worldPos.z + viewMatrix.matrix[11];

    float w = viewMatrix.matrix[12] * worldPos.x + viewMatrix.matrix[13] * worldPos.y + viewMatrix.matrix[14] * worldPos.z + viewMatrix.matrix[15];

    if (w < 0.01f) return Vec2(-1, -1);

    screenPos.x /= w;
    screenPos.y /= w;

    float x = (screenWidth / 2.0f) + (screenPos.x * screenWidth) / 2.0f;
    float y = (screenHeight / 2.0f) - (screenPos.y * screenHeight) / 2.0f;

    return Vec2(x, y);
}

class Math {
public:
    static float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    static Vec2 CalculateAngles(const Vec3& from, const Vec3& to) {
        Vec2 angles;
        float deltaX = to.x - from.x;
        float deltaY = to.y - from.y;
        float deltaZ = to.z - from.z;
        float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        angles.y = -std::atan2(deltaZ, distance) * RAD2DEG;
        angles.x = std::atan2(deltaY, deltaX) * RAD2DEG;
        return angles;
    }

    static Vec3 AngleToDirection(const Vec3& angles) {
        float sp, sy, cp, cy;
        sp = std::sin(DEG2RAD * angles.x);
        cp = std::cos(DEG2RAD * angles.x);
        sy = std::sin(DEG2RAD * angles.y);
        cy = std::cos(DEG2RAD * angles.y);
        return Vec3(cp * cy, cp * sy, -sp);
    }

    static Vec3 AngleToForwardVector(const Vec3& angles) {
        float sy = std::sin(angles.y * (M_PI / 180.0f));
        float cy = std::cos(angles.y * (M_PI / 180.0f));
        float sp = std::sin(angles.x * (M_PI / 180.0f));
        float cp = std::cos(angles.x * (M_PI / 180.0f));
        return Vec3(cp * cy, cp * sy, -sp);
    }

    static Vec3 AngleVectors(const Vec3& angles) {
        float pitch = angles.x * (M_PI / 180.0f);
        float yaw = angles.y * (M_PI / 180.0f);

        Vec3 forward;
        forward.x = std::cos(pitch) * std::cos(yaw);
        forward.y = std::cos(pitch) * std::sin(yaw);
        forward.z = -std::sin(pitch);

        return forward.Normalize();
    }
};
