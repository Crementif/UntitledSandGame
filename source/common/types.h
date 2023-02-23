#pragma once

#include <stdint.h>
#include <fstream>
#include <vector>
#include <span>
#include <algorithm>
#include <ranges>

#include <math.h>

using u64 = uint64_t;
using u32 = uint32_t;
using s32 = int32_t;
using u16 = uint16_t;
using s16 = int16_t;
using u8 = uint8_t;
using s8 = int8_t;

using f32 = float;
using f64 = double;

u16 inline _swapU16(u16 v)
{
    return (v>>8) | (v<<8);
}

s16 inline _swapS16(s16 v)
{
    return (v>>8) | (v<<8);
}

u32 inline _swapU32(u32 v)
{
    return ((v>>24)&0xff) | ((v<<8)&0xff0000) | ((v>>8)&0xff00) | ((v<<24)&0xff000000);
}

s32 inline _swapS32(s32 v)
{
    return ((v>>24)&0xff) | ((v<<8)&0xff0000) | ((v>>8)&0xff00) | ((v<<24)&0xff000000);
}

struct Vector2f
{
    Vector2f() : x(0.0f), y(0.0f) {}
    Vector2f(f32 x, f32 y) : x(x), y(y) {}

    Vector2f operator-(const Vector2f& rhs) const
    {
        return Vector2f(this->x - rhs.x, this->y - rhs.y);
    }

    Vector2f operator+(const Vector2f& rhs) const
    {
        return Vector2f(this->x + rhs.x, this->y + rhs.y);
    }

    Vector2f operator*(const f32& rhs) const
    {
        return Vector2f(this->x * rhs, this->y * rhs);
    }

    Vector2f clamp(const Vector2f& minVec, const Vector2f& maxVec) const
    {
        return {std::clamp(this->x, minVec.x, maxVec.x), std::clamp(this->y, minVec.y, maxVec.y)};
    }

    f32 DistanceSquare(const Vector2f& rhs) const
    {
        f32 dx = this->x - rhs.x;
        f32 dy = this->y - rhs.y;
        return dx*dx + dy*dy;
    }

    f32 LengthSquare() const
    {
        return x*x + y*y;
    }

    f32 Length() const
    {
        return sqrt(x*x + y*y);
    }

    Vector2f GetNormalized()
    {
        f32 s = 1.0f / sqrt(LengthSquare());
        return {x * s, y * s};
    }

    Vector2f GetNeg()
    {
        return {-x, -y};
    }

    f32 Dot(const Vector2f& rhs)
    {
        return this->x * rhs.x + this->y * rhs.y;
    }

    f32 x;
    f32 y;
};

struct Vector2i
{
    Vector2i(s32 x, s32 y) : x(x), y(y) {}

    s32 x;
    s32 y;
};

struct AABB
{
    AABB(Vector2f pos, Vector2f scale) : pos(pos), scale(scale) {}
    AABB(float x, float y, float width, float height) : pos(x, y), scale(width, height) {}

    bool Contains(const Vector2f& point) const
    {
        return point.x >= pos.x && point.x < (pos.x + scale.x) &&
                point.y >= pos.y && point.y < (pos.y + scale.y);
    }

    bool Intersects(const AABB& other) const
    {
        return (pos.x < (other.pos.x + other.scale.x) && (pos.x + scale.x) >= (other.pos.x)) &&
                (pos.y < (other.pos.y + other.scale.y) && (pos.y + scale.y) >= (other.pos.y));
    }

    Vector2f GetCenter() const
    {
        return {pos.x + scale.x * 0.5f, pos.y + scale.y * 0.5f};
    }

    // move origin of aabb by vector
    AABB operator+(const Vector2f& rhs) const
    {
        return AABB(this->pos.x + rhs.x, this->pos.y + rhs.y, this->scale.x, this->scale.y);
    }

    Vector2f pos; // left upper corner
    Vector2f scale; // width and height (must be positive)
};