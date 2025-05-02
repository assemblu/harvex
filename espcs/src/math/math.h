#pragma once

inline int screen_width = 1920;
inline int screen_height = 1080;

struct view_matrix_t
{
    float* operator[](int index)
    {
        return matrix[index];
    }

    float matrix[4][4];
};

struct Vector3
{
    float x, y, z;

    constexpr Vector3(
        const float x = 0.f,
        const float y = 0.f,
        const float z = 0.f) noexcept :
        x(x), y(y), z(z) {
    }

    constexpr Vector3 operator-(const Vector3& other) const noexcept
    {
        return Vector3{ x - other.x, y - other.y, z - other.z };
    }

    constexpr Vector3 operator+(const Vector3& other) const noexcept
    {
        return Vector3{ x + other.x, y + other.y, z + other.z };
    }

    constexpr Vector3 operator*(const float other) const noexcept
    {
        return Vector3{ x * other, y * other, z * other };
    }

    constexpr Vector3 operator/(const float other) const noexcept
    {
        return Vector3{ x / other, y / other, z / other };
    }

    Vector3 WorldToScreen(const view_matrix_t& matrix) const
    {
        float _x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3];
        float _y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3];
        float _w = matrix[3][0] * x + matrix[3][1] * y + matrix[3][2] * z + matrix[3][3];

        if (_w < 0.01f)
            return Vector3{ 0, 0, 0 };

        float invw = 1.0f / _w;
        _x *= invw;
        _y *= invw;

        float screen_x = (screen_width / 2.0f * _x) + (_x + screen_width / 2.0f);
        float screen_y = (-screen_height / 2.0f * _y) + (_y + screen_height / 2.0f);

        return Vector3{ screen_x, screen_y, z };
    }
};

struct Vector2
{
    float x, y;

    constexpr Vector2(
        const float x = 0.f,
        const float y = 0.f) noexcept :
        x(x), y(y) {
    }

    constexpr Vector2 operator-(const Vector2& other) const noexcept
    {
        return Vector2{ x - other.x, y - other.y };
    }

    constexpr Vector2 operator+(const Vector2& other) const noexcept
    {
        return Vector2{ x + other.x, y + other.y };
    }

    constexpr Vector2 operator*(const float other) const noexcept
    {
        return Vector2{ x * other, y * other };
    }

    constexpr Vector2 operator/(const float other) const noexcept
    {
        return Vector2{ x / other, y / other };
    }
};
